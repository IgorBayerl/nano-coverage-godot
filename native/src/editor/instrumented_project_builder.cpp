#include "instrumented_project_builder.h"

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

#include "../config/settings_gateway.h"
#include "../data/persistence.h"
#include "../instrumentation/instrumenter.h"

namespace godot {

namespace {

namespace fs = std::filesystem;

// Constants
const std::array<std::string_view, 8> kMandatoryCopyExtensions = {".gd", ".gdextension", ".cfg", ".dll",
                                                                  ".so", ".dylib",       ".a",   ".lib"};
const std::array<std::string_view, 2> kCriticalCacheFiles = {"uid_cache.bin", "extension_list.cfg"};
const char* kAutoloadName = "NanoCoverage";
const char* kAutoloadPath = "*res://addons/nano_coverage_godot/runtime.gd";
const char* kAddonPrefix = "addons/nano_coverage_godot/";

// Helper Predicates
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

// File Operations
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

// Pipeline Steps

std::vector<fs::path> CollectSourceFiles(const fs::path& source_root) {
    std::vector<fs::path> files;
    if (!fs::exists(source_root))
        return files;

    for (const auto& entry : fs::recursive_directory_iterator(source_root)) {
        const auto& path = entry.path();

        // We handle directory creation based on file paths
        if (fs::is_directory(path))
            continue;

        auto relative_path = fs::relative(path, source_root);
        std::string path_str = relative_path.generic_string();

        // Standard exclusions
        if (path_str.find(".godot") == 0 || path_str.find(".git") == 0)
            continue;
        if (IsHotReloadArtifact(path.filename().string()))
            continue;

        files.push_back(path);
    }
    return files;
}

void CopyOrLinkFile(const fs::path& src, const fs::path& dst) {
    std::error_code ec;
    // Note: create_directories(parent) is done by caller to avoid repeated checks here

    if (IsCopyMandatory(src)) {
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    } else {
        CreateSymlinkOrCopy(src, dst);
    }
}

void InstrumentFileIfNeeded(const fs::path& target_file, const fs::path& relative_path, CoverageMetadata& metadata) {
    if (target_file.extension() != ".gd")
        return;

    if (!ShouldInstrumentFile(relative_path))
        return;

    std::string res_gd = "res://" + relative_path.generic_string();
    std::replace(res_gd.begin(), res_gd.end(), '\\', '/');

    int insertions = 0;
    std::vector<uint32_t> lines;

    Instrumenter::instrument_file(String(target_file.string().c_str()), String(res_gd.c_str()), &lines, &insertions);

    if (!lines.empty()) {
        metadata[res_gd] = std::move(lines);
    }
}

void SyncGodotCache(const fs::path& src_root, const fs::path& dst_root) {
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

void PatchProjectSettings(const fs::path& temp_project_root, const fs::path& original_project_root) {
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

String InstrumentedProjectBuilder::build_instrumented_project() {
    // Resolve Paths
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

    // Prepare Temp Directory
    if (fs::exists(dest_root))
        fs::remove_all(dest_root, ec);
    fs::create_directories(dest_root, ec);

    UtilityFunctions::print("NanoCoverage: Building temp project at ", String(dest_root.string().c_str()));

    // Process Files (Collect, Copy, Instrument)
    CoverageMetadata global_metadata;
    auto files = CollectSourceFiles(source_root);

    for (const auto& src_path : files) {
        auto relative = fs::relative(src_path, source_root);
        auto target = dest_root / relative;

        fs::create_directories(target.parent_path(), ec);

        CopyOrLinkFile(src_path, target);

        if (IsCopyMandatory(src_path)) {
            InstrumentFileIfNeeded(target, relative, global_metadata);
        }
    }

    // Sync Cache
    SyncGodotCache(source_root, dest_root);

    // Finalize Configuration
    if (fs::exists(dest_root / "project.godot")) {
        PatchProjectSettings(dest_root, source_root);

        // Save Metadata to Disk
        CoverageSettings settings = SettingsGateway::load();
        String data_store_dir = settings.paths_data_store_dir;

        String global_data_dir = ProjectSettings::get_singleton()->globalize_path(data_store_dir);
        fs::path meta_dir(global_data_dir.utf8().get_data());
        std::error_code ec_meta;
        fs::create_directories(meta_dir, ec_meta);

        fs::path meta_path = meta_dir / "coverage.meta";

        UtilityFunctions::print("NanoCoverage: Saving metadata to ", String(meta_path.string().c_str()));
        if (!Persistence::save_metadata(meta_path.string(), global_metadata)) {
            UtilityFunctions::printerr("NanoCoverage: Failed to save coverage.meta!");
        }
    } else {
        UtilityFunctions::printerr("NanoCoverage: CRITICAL - project.godot not found in temp directory!");
        return "";
    }

    std::string final_path = dest_root.string();
    std::replace(final_path.begin(), final_path.end(), '\\', '/');
    return String(final_path.c_str());
}

}  // namespace godot