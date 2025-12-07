#include "temp_builder.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

namespace godot {

// Helper to decide if we MUST copy the file (because we will modify it)
bool is_copy_mandatory(const fs::path& p) {
    // We must copy scripts to instrument them later
    if (p.extension() == ".gd") return true;
    // We must copy project configuration to patch autoloads
    if (p.filename() == "project.godot") return true;
    return false;
}

String TempProjectBuilder::create_temp_project() {
    // 1. Determine Source Path
    String res_path = ProjectSettings::get_singleton()->globalize_path("res://");
    fs::path source_path(res_path.utf8().get_data());
    
    // 2. Determine Destination Root
    fs::path temp_root;
    String custom_path_setting = ProjectSettings::get_singleton()->get_setting("nano_coverage/general/temp_directory");
    
    if (!custom_path_setting.is_empty()) {
        String global_custom = ProjectSettings::get_singleton()->globalize_path(custom_path_setting);
        temp_root = fs::path(global_custom.utf8().get_data());
    } else {
        temp_root = fs::temp_directory_path() / "nano_coverage_godot_runs";
    }

    // Create a unique subfolder based on the project path hash
    std::string project_hash = std::to_string(std::hash<std::string>{}(source_path.string()));
    fs::path dest_path = temp_root / project_hash;

    UtilityFunctions::print("NanoCoverage: Source: ", String(source_path.string().c_str()));
    UtilityFunctions::print("NanoCoverage: Dest: ", String(dest_path.string().c_str()));

    std::error_code ec;
    
    // 3. Clean Previous Run
    if (fs::exists(dest_path)) {
        fs::remove_all(dest_path, ec);
        if (ec) {
            UtilityFunctions::printerr("NanoCoverage: Failed to clean temp dir: ", String(ec.message().c_str()));
            return "";
        }
    }
    fs::create_directories(dest_path, ec);

    // 4. Main Project Recursive Copy/Symlink
    for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, source_path);
        std::string path_str = relative_path.string();
        
        // Skip .godot and .git folders
        if (path_str.find(".godot") == 0 || path_str.find(".git") == 0) {
            continue;
        }

        fs::path target = dest_path / relative_path;

        if (fs::is_directory(path)) {
            fs::create_directories(target, ec);
            if (ec) UtilityFunctions::printerr("NanoCoverage: Dir error: ", String(ec.message().c_str()));
        } else {
            if (is_copy_mandatory(path)) {
                fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
                if (ec) UtilityFunctions::printerr("NanoCoverage: Copy error: ", String(ec.message().c_str()));
            } else {
                // Try Symlink first
                fs::create_symlink(path, target, ec);
                if (ec) {
                    ec.clear();
                    fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
                }
            }
        }
    }

    // 5. Essential Cache & Artifacts (.godot folder)
    fs::path src_godot_dir = source_path / ".godot";
    fs::path dest_godot_dir = dest_path / ".godot";
    
    if (fs::exists(src_godot_dir)) {
        fs::create_directories(dest_godot_dir, ec);

        // 5a. Copy Critical Cache Files
        std::vector<std::string> critical_cache_files = {
            "uid_cache.bin",
            "global_script_class_cache.cfg"
        };

        for (const auto& filename : critical_cache_files) {
            fs::path src_file = src_godot_dir / filename;
            fs::path dest_file = dest_godot_dir / filename;

            if (fs::exists(src_file)) {
                fs::copy_file(src_file, dest_file, fs::copy_options::overwrite_existing, ec);
                if (!ec) {
                    UtilityFunctions::print("NanoCoverage: Copied cache file: ", String(filename.c_str()));
                }
            }
        }

        // 5b. Sync Imported Artifacts
        fs::path src_imported = src_godot_dir / "imported";
        
        if (fs::exists(src_imported)) {
            // Explicitly create the destination 'imported' folder first
            fs::create_directories(dest_godot_dir / "imported", ec);

            for (const auto& entry : fs::recursive_directory_iterator(src_imported)) {
                const auto& path = entry.path();
                auto relative = fs::relative(path, src_godot_dir); // e.g., "imported/foo.ctex"
                fs::path target = dest_godot_dir / relative;

                if (fs::is_directory(path)) {
                    fs::create_directories(target, ec);
                } else {
                    // Try Symlink first
                    fs::create_symlink(path, target, ec);
                    if (ec) {
                        ec.clear();
                        fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
                        
                        // Log error if fallback copy also fails
                        if (ec) {
                             UtilityFunctions::printerr("NanoCoverage: Import sync failed for ", String(path.string().c_str()), ": ", String(ec.message().c_str()));
                        }
                    }
                }
            }
            UtilityFunctions::print("NanoCoverage: Synced .godot/imported artifacts.");
        }
    }

    // 6. Verification
    if (!fs::exists(dest_path / "project.godot")) {
        UtilityFunctions::printerr("NanoCoverage: CRITICAL - project.godot not found in temp directory!");
        return "";
    }

    // 7. Path Normalization
    std::string final_path_str = dest_path.string();
    std::replace(final_path_str.begin(), final_path_str.end(), '\\', '/');
    
    return String(final_path_str.c_str());
}

} // namespace godot