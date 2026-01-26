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
#include <string_view>
#include <vector>

#include "../instrumentation/instrumenter.h"

namespace godot {

namespace {

namespace fs = std::filesystem;

// Files with these extensions must be physically copied to the temp directory.
// Symlinking them can cause file locking (DLLs), permission errors (.cfg),
// or prevents us from modifying them in-place (.gd).
const std::array<std::string_view, 8> kMandatoryCopyExtensions = {".gd", ".gdextension", ".cfg", ".dll",
                                                                  ".so", ".dylib",       ".a",   ".lib"};

// Cache files required for the project to run correctly without the Editor.
const std::array<std::string_view, 2> kCriticalCacheFiles = {
    "uid_cache.bin",      // Resolves uid:// paths
    "extension_list.cfg"  // Tells the runtime to load our GDExtension
};

// Constants for configuration injection
const char* kAutoloadName = "NanoCoverage";
const char* kAutoloadPath = "*res://addons/nano_coverage_godot/runtime.gd";
const char* kAddonPrefix = "addons/nano_coverage_godot/";

// --- Helper Functions ---

bool IsEnvTruthy(const char* name) {
    const char* v = std::getenv(name);
    return v && *v && std::string_view(v) != "0";
}

bool IsHotReloadArtifact(const std::string& filename) {
    // Windows creates temp files like "~filename.dll" during hot-reload.
    return !filename.empty() && filename[0] == '~';
}

bool ShouldInstrumentFile(const fs::path& relative_path) {
    // Allow overriding via env var for debugging the plugin itself
    if (IsEnvTruthy("NANO_COVERAGE_INSTRUMENT_ADDONS")) {
        return true;
    }

    // Do not instrument the coverage tool's own code
    std::string rel_str = relative_path.generic_string();
    if (rel_str.find(kAddonPrefix) == 0) {
        return false;
    }

    return true;
}

bool IsCopyMandatory(const fs::path& path) {
    std::string filename = path.filename().string();

    // Always ignore hot-reload trash
    if (IsHotReloadArtifact(filename))
        return false;

    // Project file must be copied to be mutable
    if (filename == "project.godot")
        return true;

    std::string ext = path.extension().string();
    for (const auto& mandatory_ext : kMandatoryCopyExtensions) {
        if (ext == mandatory_ext)
            return true;
    }

    return false;
}

// Tries to create a symlink; falls back to a physical copy on failure.
void CreateSymlinkOrCopy(const fs::path& src, const fs::path& dst) {
    std::error_code ec;
    fs::create_symlink(src, dst, ec);

    if (ec) {
        // Fallback: Copy
        ec.clear();
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            UtilityFunctions::printerr("NanoCoverage: Failed to link/copy ", String(src.string().c_str()), " -> ",
                                       String(ec.message().c_str()));
        }
    }
}

// Copies essential data from .godot/ to ensure assets and extensions load.
void SyncGodotCache(const fs::path& src_root, const fs::path& dst_root) {
    fs::path src_godot = src_root / ".godot";
    fs::path dst_godot = dst_root / ".godot";

    if (!fs::exists(src_godot))
        return;

    std::error_code ec;
    fs::create_directories(dst_godot, ec);

    // 1. Copy Critical Cache Files (uid resolution, extension loading)
    for (const auto& filename : kCriticalCacheFiles) {
        fs::path src_file = src_godot / filename;
        fs::path dst_file = dst_godot / filename;

        if (fs::exists(src_file)) {
            fs::copy_file(src_file, dst_file, fs::copy_options::overwrite_existing, ec);
        }
    }

    // 2. Sync Imported Artifacts (Textures, Sounds, etc.)
    // We iterate recursively and use the Symlink/Copy helper.
    fs::path src_imported = src_godot / "imported";
    if (fs::exists(src_imported)) {
        fs::create_directories(dst_godot / "imported", ec);

        for (const auto& entry : fs::recursive_directory_iterator(src_imported)) {
            const auto& path = entry.path();
            auto relative = fs::relative(path, src_godot);
            fs::path target = dst_godot / relative;

            if (fs::is_directory(path)) {
                fs::create_directories(target, ec);
            } else {
                CreateSymlinkOrCopy(path, target);
            }
        }
    }
}

// Modifies project.godot to ensure it runs in the isolated environment.
void SanitizeProjectConfig(const fs::path& project_root) {
    String project_file_str = String((project_root / "project.godot").string().c_str());

    Ref<ConfigFile> cfg;
    cfg.instantiate();

    if (cfg->load(project_file_str) != OK) {
        UtilityFunctions::printerr("NanoCoverage: Failed to load temp project.godot");
        return;
    }

    // 1. Fix Main Scene (UID -> Absolute Path)
    // Even though we copy uid_cache.bin, resolving this explicitly makes startup more robust
    // if the cache is slightly out of date.
    String main_scene = ProjectSettings::get_singleton()->get_setting("application/run/main_scene");

    if (main_scene.begins_with("uid://")) {
        int64_t uid = ResourceUID::get_singleton()->text_to_id(main_scene);
        if (uid != -1) {
            String resolved = ResourceUID::get_singleton()->get_id_path(uid);
            if (!resolved.is_empty()) {
                main_scene = resolved;
            }
        }
    }
    cfg->set_value("application", "run/main_scene", main_scene);

    // 2. Inject Autoload
    // This allows the instrumented code (NanoCoverage.hit) to find the runtime singleton.
    cfg->set_value("autoload", kAutoloadName, kAutoloadPath);

    cfg->save(project_file_str);
}

}  // namespace

String TempProjectBuilder::create_temp_project() {
    // 1. Resolve Paths
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

    // Generate hash based on source path to separate different projects
    std::string project_hash = std::to_string(std::hash<std::string>{}(source_root.string()));
    fs::path dest_root = temp_root / project_hash;

    // 2. Clean Destination
    std::error_code ec;
    if (fs::exists(dest_root)) {
        fs::remove_all(dest_root, ec);
    }
    fs::create_directories(dest_root, ec);

    UtilityFunctions::print("NanoCoverage: Building temp project at ", String(dest_root.string().c_str()));

    // 3. Recursive Copy / Instrument
    for (const auto& entry : fs::recursive_directory_iterator(source_root)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, source_root);
        std::string path_str = relative_path.generic_string();

        // Skip internal .godot (handled separately) and .git
        if (path_str.find(".godot") == 0 || path_str.find(".git") == 0)
            continue;

        // Skip hot-reload artifacts
        if (IsHotReloadArtifact(path.filename().string()))
            continue;

        fs::path target = dest_root / relative_path;

        if (fs::is_directory(path)) {
            fs::create_directories(target, ec);
            continue;
        }

        if (IsCopyMandatory(path)) {
            // Physical Copy
            fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);

            // Instrumentation Logic
            if (path.extension() == ".gd" && ShouldInstrumentFile(relative_path)) {
                std::string res_gd = "res://" + relative_path.generic_string();
                std::replace(res_gd.begin(), res_gd.end(), '\\', '/');  // Normalize slashes for Godot

                int insertions = 0;
                Instrumenter::instrument_file_in_place(target, res_gd, &insertions);
            }
        } else {
            // Symlink (Fast)
            CreateSymlinkOrCopy(path, target);
        }
    }

    // 4. Sync Cache & Imports (Crucial step for runtime execution)
    SyncGodotCache(source_root, dest_root);

    // 5. Finalize Configuration
    if (fs::exists(dest_root / "project.godot")) {
        SanitizeProjectConfig(dest_root);
    } else {
        UtilityFunctions::printerr("NanoCoverage: CRITICAL - project.godot not found in temp directory!");
        return "";
    }

    // Return normalized string path
    std::string final_path = dest_root.string();
    std::replace(final_path.begin(), final_path.end(), '\\', '/');

    return String(final_path.c_str());
}

}  // namespace godot