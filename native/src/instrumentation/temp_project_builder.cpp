#include "nano_coverage/temp_project_builder.hpp"

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

String TempProjectBuilder::create_temp_project() {
    String res_path = ProjectSettings::get_singleton()->globalize_path("res://");
    fs::path source_path(res_path.utf8().get_data());
    
    fs::path temp_root = fs::temp_directory_path() / "nano_coverage_godot_runs";
    std::string project_hash = std::to_string(std::hash<std::string>{}(source_path.string()));
    fs::path dest_path = temp_root / project_hash;

    UtilityFunctions::print("NanoCoverage: Source: ", String(source_path.string().c_str()));
    UtilityFunctions::print("NanoCoverage: Dest: ", String(dest_path.string().c_str()));

    std::error_code ec;
    
    // 1. Clean previous run
    if (fs::exists(dest_path)) {
        fs::remove_all(dest_path, ec);
        if (ec) {
            UtilityFunctions::printerr("NanoCoverage: Failed to clean temp dir: ", String(ec.message().c_str()));
            return "";
        }
    }
    fs::create_directories(dest_path, ec);

    // 2. Recursive Copy (Skipping .godot and .git folders)
    for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, source_path);
        std::string path_str = relative_path.string();
        
        // Skip .godot and .git folders in the main loop
        if (path_str.find(".godot") == 0 || path_str.find(".git") == 0) {
            continue;
        }

        fs::path target = dest_path / relative_path;

        if (fs::is_directory(path)) {
            fs::create_directories(target, ec);
        } else {
            fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
        }
    }

    // 3. ESSENTIAL: Manually copy the UID cache and Class cache
    // Without these, Godot fails to resolve UIDs (like the main scene) on the first run.
    fs::path src_godot_dir = source_path / ".godot";
    fs::path dest_godot_dir = dest_path / ".godot";
    
    if (fs::exists(src_godot_dir)) {
        fs::create_directories(dest_godot_dir, ec);

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
    }

    // 4. Verification
    if (!fs::exists(dest_path / "project.godot")) {
        UtilityFunctions::printerr("NanoCoverage: CRITICAL - project.godot not found in temp directory!");
        return "";
    }

    // 5. Return Normalized Path
    std::string final_path_str = dest_path.string();
    std::replace(final_path_str.begin(), final_path_str.end(), '\\', '/');
    
    return String(final_path_str.c_str());
}

} // namespace godot