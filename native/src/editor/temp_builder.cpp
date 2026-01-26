#include "temp_builder.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>
#include <vector>

#include "../instrumentation/instrumenter.h"

namespace fs = std::filesystem;

namespace godot {

// Helper to decide if we MUST copy the file
static bool is_copy_mandatory(const fs::path& p) {
    std::string filename = p.filename().string();

    // 1. Ignore Hot-Reload Artifacts (Windows)
    if (!filename.empty() && filename[0] == '~')
        return false;

    std::string ext = p.extension().string();

    // 2. Scripts and Project settings
    if (ext == ".gd")
        return true;
    if (filename == "project.godot")
        return true;

    // 3. GDExtension artifacts and Configs
    if (ext == ".gdextension")
        return true;
    if (ext == ".cfg")
        return true;

    // 4. Binaries
    if (ext == ".dll" || ext == ".so" || ext == ".dylib" || ext == ".a" || ext == ".lib")
        return true;

    return false;
}

static bool env_truthy(const char* name) {
    const char* v = std::getenv(name);
    if (!v || !*v)
        return false;
    return std::string_view(v) != "0";
}

static bool should_instrument_rel_path(const fs::path& relative_path) {
    if (env_truthy("NANO_COVERAGE_INSTRUMENT_ADDONS"))
        return true;

    // Don't instrument our own addon
    const std::string rel = relative_path.generic_string();
    if (rel.rfind("addons/nano_coverage_godot/", 0) == 0)
        return false;

    return true;
}

static void sanitize_project_config(const fs::path& dest_root) {
    String project_file_str = String((dest_root / "project.godot").string().c_str());

    Ref<ConfigFile> cfg;
    cfg.instantiate();

    Error err = cfg->load(project_file_str);
    if (err != OK) {
        UtilityFunctions::printerr("NanoCoverage: Failed to load temp project.godot for editing.");
        return;
    }

    // 1. Fix Main Scene (Replace UID with absolute res:// path)
    // This allows the temp project to run without a full resource scan/cache.
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

    // 2. Configure Autoload
    // Key "NanoCoverage" matches the injected "NanoCoverage.hit()" calls.
    cfg->set_value("autoload", "NanoCoverage", "*res://addons/nano_coverage_godot/runtime.gd");

    cfg->save(project_file_str);
    UtilityFunctions::print("NanoCoverage: Sanitized project.godot (Main Scene: ", main_scene, ")");
}

String TempProjectBuilder::create_temp_project() {
    String res_path = ProjectSettings::get_singleton()->globalize_path("res://");
    fs::path source_path(res_path.utf8().get_data());

    // Determine Destination
    fs::path temp_root;
    String custom_path_setting = ProjectSettings::get_singleton()->get_setting("nano_coverage/general/temp_directory");

    if (!custom_path_setting.is_empty()) {
        String global_custom = ProjectSettings::get_singleton()->globalize_path(custom_path_setting);
        temp_root = fs::path(global_custom.utf8().get_data());
    } else {
        temp_root = fs::temp_directory_path() / "nano_coverage_godot_runs";
    }

    std::string project_hash = std::to_string(std::hash<std::string>{}(source_path.string()));
    fs::path dest_path = temp_root / project_hash;

    std::error_code ec;

    // Clean Previous Run
    if (fs::exists(dest_path)) {
        fs::remove_all(dest_path, ec);
    }
    fs::create_directories(dest_path, ec);

    // Main Copy Loop
    for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, source_path);
        std::string path_str = relative_path.generic_string();

        if (path_str.rfind(".godot", 0) == 0 || path_str.rfind(".git", 0) == 0)
            continue;
        if (!path.filename().string().empty() && path.filename().string()[0] == '~')
            continue;

        fs::path target = dest_path / relative_path;

        if (fs::is_directory(path)) {
            fs::create_directories(target, ec);
            continue;
        }

        if (is_copy_mandatory(path)) {
            fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);

            // Instrument .gd
            if (path.extension() == ".gd" && should_instrument_rel_path(relative_path)) {
                const std::string rel = relative_path.generic_string();
                const std::string res_gd = "res://" + rel;
                int insertions = 0;
                Instrumenter::instrument_file_in_place(target, res_gd, &insertions);
            }
        } else {
            // Symlink with fallback
            fs::create_symlink(path, target, ec);
            if (ec) {
                ec.clear();
                fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
            }
        }
    }

    // Handle .godot folder
    fs::path src_godot_dir = source_path / ".godot";
    fs::path dest_godot_dir = dest_path / ".godot";

    if (fs::exists(src_godot_dir)) {
        fs::create_directories(dest_godot_dir, ec);

        // 1. Copy Critical Cache/Config Files
        // - uid_cache.bin: Needed for UIDs (though we sanitize main_scene, other resources might need it)
        // - extension_list.cfg: TELLS THE ENGINE TO LOAD THE GDEXTENSIONS!
        std::vector<std::string> critical_files = {"uid_cache.bin", "extension_list.cfg"};

        for (const auto& filename : critical_files) {
            fs::path src_file = src_godot_dir / filename;
            fs::path dest_file = dest_godot_dir / filename;

            if (fs::exists(src_file)) {
                fs::copy_file(src_file, dest_file, fs::copy_options::overwrite_existing, ec);
            }
        }

        // 2. Sync Imported Artifacts
        fs::path src_imported = src_godot_dir / "imported";
        if (fs::exists(src_imported)) {
            fs::create_directories(dest_godot_dir / "imported", ec);
            for (const auto& entry : fs::recursive_directory_iterator(src_imported)) {
                const auto& path = entry.path();
                auto relative = fs::relative(path, src_godot_dir);
                fs::path target = dest_godot_dir / relative;

                if (fs::is_directory(path)) {
                    fs::create_directories(target, ec);
                } else {
                    fs::create_symlink(path, target, ec);
                    if (ec) {
                        ec.clear();
                        fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
                    }
                }
            }
        }
    }

    // Verify Project Exists
    if (!fs::exists(dest_path / "project.godot")) {
        UtilityFunctions::printerr("NanoCoverage: CRITICAL - project.godot not found in temp directory!");
        return "";
    }

    // Sanitize Project Config (Fix UIDs, Add Autoload)
    sanitize_project_config(dest_path);

    std::string final_path_str = dest_path.string();
    std::replace(final_path_str.begin(), final_path_str.end(), '\\', '/');

    return String(final_path_str.c_str());
}

}  // namespace godot