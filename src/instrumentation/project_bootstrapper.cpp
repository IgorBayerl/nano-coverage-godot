#include "project_bootstrapper.h"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include "../config/settings_gateway.h"
#include "../api/coverage_api.h"
#include "../utils/logger.h"

namespace godot {

void ProjectBootstrapper::_bind_methods() {
    ClassDB::bind_method(D_METHOD("instrument_all_scripts"), &ProjectBootstrapper::instrument_all_scripts);
}

Array ProjectBootstrapper::compile_ignore_patterns(const Array& glob_patterns) {
    Array compiled_regexes;
    for (int i = 0; i < glob_patterns.size(); ++i) {
        String glob = glob_patterns[i];
        
        // Convert Glob to strict Regex
        // 1. Escape dots
        String regex_str = glob.replace(".", "\\.");
        // 2. Replace ** with .* (matches across slashes)
        regex_str = regex_str.replace("**", ".*");
        // 3. Replace single * with [^/]* (matches inside a single directory)
        // Note: careful to avoid replacing the .* we just made
        regex_str = regex_str.replace("[^/]*[^/]*", ".*"); // Cleanup edge cases
        
        regex_str = "^" + regex_str + "$";

        Ref<RegEx> rx = RegEx::create_from_string(regex_str);
        if (rx.is_valid()) {
            compiled_regexes.push_back(rx);
        }
    }
    return compiled_regexes;
}

Array ProjectBootstrapper::get_all_files(const String& current_path, const Array& compiled_regexes) {
    Array files;
    Ref<DirAccess> dir = DirAccess::open(current_path);
    
    if (dir.is_null()) {
        Logger::error("Bootstrapper failed to open directory: " + current_path);
        return files;
    }

    dir->list_dir_begin();
    String file_name = dir->get_next();

    while (!file_name.is_empty()) {
        if (file_name == "." || file_name == "..") {
            file_name = dir->get_next();
            continue;
        }

        String full_path = current_path.path_join(file_name);
        bool is_ignored = false;

        for (int i = 0; i < compiled_regexes.size(); ++i) {
            Ref<RegEx> rx = compiled_regexes[i];
            if (rx->search(full_path).is_valid()) {
                is_ignored = true;
                break;
            }
        }

        if (is_ignored) {
            file_name = dir->get_next();
            continue;
        }

        if (dir->current_is_dir()) {
            files.append_array(get_all_files(full_path, compiled_regexes));
        } else if (file_name.ends_with(".gd")) {
            files.append(full_path);
        }

        file_name = dir->get_next();
    }
    return files;
}

void ProjectBootstrapper::instrument_all_scripts() {
    Logger::info("--- Starting Memory Instrumentation ---");

    CoverageSettings settings = SettingsGateway::load();
    Array raw_ignores;
    
    for (int i = 0; i < settings.ignore_paths.size(); ++i) {
        raw_ignores.push_back(settings.ignore_paths[i]);
    }

    if (settings.ignore_addons) {
        raw_ignores.push_back("res://addons/**");
    } else {
        // Hardcoded ignore for nanocoverage gdscripts. Cannot be instrumented anyway.
        raw_ignores.push_back("res://addons/nano_coverage_godot/**");
    }

    Array compiled_ignores = compile_ignore_patterns(raw_ignores);
    Array files = get_all_files("res://", compiled_ignores);

    Logger::info("Total GDScript files found to instrument: " + String::num_int64(files.size()));

    Ref<CoverageApi> api;
    api.instantiate();
    int instrumented_count = 0;
    int ignored_count = 0;

    for (int i = 0; i < files.size(); ++i) {
        String path = files[i];
        Ref<GDScript> script = ResourceLoader::get_singleton()->load(path);
        
        if (script.is_null()) continue;

        Dictionary res = api->instrument_script(script->get_source_code(), path);
        
        if (res.has("success") && bool(res["success"])) {
            if (res.has("ignored") && bool(res["ignored"])) {
                ignored_count++;
            } else {
                script->set_source_code(res["code"]);
                script->reload(true);
                instrumented_count++;
            }
        } else {
            Logger::error("Failed to instrument: " + path);
        }
    }

    Logger::info("Saving static coverage metadata...");
    api->save_static_metadata();
    
    Logger::info("--- Instrumentation Complete ---");
    Logger::info("Scripts Patched: " + String::num_int64(instrumented_count));
    Logger::info("Scripts Ignored (0 Lines): " + String::num_int64(ignored_count));
}

} // namespace godot
