#include "temp_builder.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>
#include <vector>

#include "../data/persistence.h"
#include "../instrumentation/instrumenter.h"

namespace godot {

namespace {
// ... [Retain existing constants and helpers: fs, kMandatoryCopyExtensions, IsEnvTruthy, etc.] ...
namespace fs = std::filesystem;
const std::array<std::string_view, 8> kMandatoryCopyExtensions = {".gd", ".gdextension", ".cfg", ".dll",
                                                                  ".so", ".dylib",       ".a",   ".lib"};
const std::array<std::string_view, 2> kCriticalCacheFiles = {"uid_cache.bin", "extension_list.cfg"};
const char* kAutoloadName = "NanoCoverage";
const char* kAutoloadPath = "*res://addons/nano_coverage_godot/runtime.gd";
const char* kAddonPrefix = "addons/nano_coverage_godot/";

bool IsEnvTruthy(const char* name) {
    const char* v = std::getenv(name);
    return v && *v && std::string_view(v) != "0";
}

bool IsHotReloadArtifact(const std::string& filename) {
    return !filename.empty() && filename[0] == '~';
}

bool ShouldInstrumentFile(const fs::path& relative_path) {
    if (IsEnvTruthy("NANO_COVERAGE_INSTRUMENT_ADDONS"))
        return true;
    std::string rel_str = relative_path.generic_string();
    return rel_str.find(kAddonPrefix) != 0;
}

bool IsCopyMandatory(const fs::path& path) {
    std::string filename = path.filename().string();
    if (IsHotReloadArtifact(filename))
        return false;
    if (filename == "project.godot")
        return true;
    std::string ext = path.extension().string();
    for (const auto& mandatory_ext : kMandatoryCopyExtensions)
        if (ext == mandatory_ext)
            return true;
    return false;
}

void CreateSymlinkOrCopy(const fs::path& src, const fs::path& dst) {
    std::error_code ec;
    fs::create_symlink(src, dst, ec);
    if (ec) {
        ec.clear();
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (ec)
            UtilityFunctions::printerr("NanoCoverage: Failed to link/copy ", String(src.string().c_str()));
    }
}

void SyncGodotCache(const fs::path& src_root, const fs::path& dst_root) {
    // ... [Same implementation as before] ...
    fs::path src_godot = src_root / ".godot";
    fs::path dst_godot = dst_root / ".godot";
    if (!fs::exists(src_godot))
        return;
    std::error_code ec;
    fs::create_directories(dst_godot, ec);
    for (const auto& filename : kCriticalCacheFiles) {
        fs::path src_file = src_godot / filename;
        fs::path dst_file = dst_godot / filename;
        if (fs::exists(src_file))
            fs::copy_file(src_file, dst_file, fs::copy_options::overwrite_existing, ec);
    }
    fs::path src_imported = src_godot / "imported";
    if (fs::exists(src_imported)) {
        fs::create_directories(dst_godot / "imported", ec);
        for (const auto& entry : fs::recursive_directory_iterator(src_imported)) {
            const auto& path = entry.path();
            auto relative = fs::relative(path, src_godot);
            fs::path target = dst_godot / relative;
            if (fs::is_directory(path))
                fs::create_directories(target, ec);
            else
                CreateSymlinkOrCopy(path, target);
        }
    }
}

void SanitizeProjectConfig(const fs::path& temp_project_root, const fs::path& original_project_root) {
    String project_file_str = String((temp_project_root / "project.godot").string().c_str());
    Ref<ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load(project_file_str) != OK)
        return;

    String main_scene = ProjectSettings::get_singleton()->get_setting("application/run/main_scene");
    if (main_scene.begins_with("uid://")) {
        int64_t uid = ResourceUID::get_singleton()->text_to_id(main_scene);
        if (uid != -1) {
            String resolved = ResourceUID::get_singleton()->get_id_path(uid);
            if (!resolved.is_empty())
                main_scene = resolved;
        }
    }
    cfg->set_value("application", "run/main_scene", main_scene);
    cfg->set_value("autoload", kAutoloadName, kAutoloadPath);

    // Prepare paths for runtime output
    std::string source_root_str = original_project_root.string();
    std::replace(source_root_str.begin(), source_root_str.end(), '\\', '/');

    // Default output directory is the source project root
    cfg->set_value("nano_coverage", "output_dir", String(source_root_str.c_str()));
    cfg->set_value("nano_coverage", "source_root", String(source_root_str.c_str()));

    cfg->save(project_file_str);
}
}  // namespace

String TempProjectBuilder::create_temp_project() {
    String res_path_gd = ProjectSettings::get_singleton()->globalize_path("res://");
    fs::path source_root(res_path_gd.utf8().get_data());
    fs::path temp_root;
    String custom_path_setting = ProjectSettings::get_singleton()->get_setting("nano_coverage/general/temp_directory");

    if (!custom_path_setting.is_empty()) {
        String global_custom = ProjectSettings::get_singleton()->globalize_path(custom_path_setting);
        temp_root = fs::path(global_custom.utf8().get_data());
    } else {
        temp_root = fs::temp_directory_path() / "nano_coverage_godot_runs";
    }

    std::string project_hash = std::to_string(std::hash<std::string>{}(source_root.string()));
    fs::path dest_root = temp_root / project_hash;
    std::error_code ec;

    if (fs::exists(dest_root))
        fs::remove_all(dest_root, ec);
    fs::create_directories(dest_root, ec);

    UtilityFunctions::print("NanoCoverage: Building temp project at ", String(dest_root.string().c_str()));

    // NEW: Accumulate metadata during the copy/instrument loop
    CoverageMetadata global_metadata;

    for (const auto& entry : fs::recursive_directory_iterator(source_root)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, source_root);
        std::string path_str = relative_path.generic_string();

        if (path_str.find(".godot") == 0 || path_str.find(".git") == 0)
            continue;
        if (IsHotReloadArtifact(path.filename().string()))
            continue;

        fs::path target = dest_root / relative_path;
        if (fs::is_directory(path)) {
            fs::create_directories(target, ec);
            continue;
        }

        if (IsCopyMandatory(path)) {
            fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);

            if (path.extension() == ".gd" && ShouldInstrumentFile(relative_path)) {
                std::string res_gd = "res://" + relative_path.generic_string();
                std::replace(res_gd.begin(), res_gd.end(), '\\', '/');

                int insertions = 0;
                std::vector<uint32_t> lines;

                Instrumenter::instrument_file_in_place(target, res_gd, lines, &insertions);

                // Store metadata for this file
                if (!lines.empty()) {
                    global_metadata[res_gd] = std::move(lines);
                }
            }
        } else {
            CreateSymlinkOrCopy(path, target);
        }
    }

    SyncGodotCache(source_root, dest_root);

    if (fs::exists(dest_root / "project.godot")) {
        SanitizeProjectConfig(dest_root, source_root);

        // NEW: Save the accumulated metadata to the original source root
        fs::path meta_path = source_root / "coverage.meta";
        UtilityFunctions::print("NanoCoverage: Saving metadata to ", String(meta_path.string().c_str()));
        Persistence::save_metadata(meta_path.string(), global_metadata);

    } else {
        UtilityFunctions::printerr("NanoCoverage: CRITICAL - project.godot not found in temp directory!");
        return "";
    }

    std::string final_path = dest_root.string();
    std::replace(final_path.begin(), final_path.end(), '\\', '/');
    return String(final_path.c_str());
}

}  // namespace godot