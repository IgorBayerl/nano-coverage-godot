#include "project_bootstrapper.h"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/classes/project_settings.hpp> // <-- Added for Autoload detection
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
        
        String regex_str = glob.replace(".", "\\.");
        regex_str = regex_str.replace("**", "<<GLOBSTAR>>");
        regex_str = regex_str.replace("*", "[^/]*");
        regex_str = regex_str.replace("<<GLOBSTAR>>", ".*");
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
        raw_ignores.push_back("res://addons/nano_coverage_godot/**");
    }

    Array compiled_ignores = compile_ignore_patterns(raw_ignores);
    Array files = get_all_files("res://", compiled_ignores);
    Logger::info("Total GDScript files found to instrument: " + String::num_int64(files.size()));

    // --- AUTOLOAD DETECTION ---
    // Godot stores autoloads in ProjectSettings. They often start with a '*' if they are singletons.
    Array autoload_paths;
    TypedArray<Dictionary> props = ProjectSettings::get_singleton()->get_property_list();
    for (int i = 0; i < props.size(); i++) {
        Dictionary prop = props[i];
        String name = prop["name"];
        if (name.begins_with("autoload/")) {
            String path = ProjectSettings::get_singleton()->get_setting(name);
            // Autoloads are sometimes formatted as "*res://path/to/script.gd"
            if (path.begins_with("*")) {
                path = path.substr(1); 
            }
            autoload_paths.push_back(path);
        }
    }

    Ref<CoverageApi> api;
    api.instantiate();
    int instrumented_count = 0;
    int ignored_count = 0;
    int failed_count = 0;

    for (int i = 0; i < files.size(); ++i) {
        String path = files[i];

        // --- AUTOLOAD BYPASS ---
        if (autoload_paths.has(path)) {
            Logger::info("Skipping Autoload/Singleton to preserve game state: " + path);
            ignored_count++;
            continue;
        }

        Ref<GDScript> script = ResourceLoader::get_singleton()->load(path);
        if (script.is_null()) {
            continue;
        }

        String original_code = script->get_source_code();
        Dictionary res = api->instrument_script(original_code, path);
        
        if (!res.has("success") || !bool(res["success"])) {
            Logger::error("Failed to instrument: " + path);
            failed_count++;
            continue;
        }
        
        if (res.has("ignored") && bool(res["ignored"])) {
            ignored_count++;
            continue;
        }
        
        String new_code = res["code"];
        script->set_source_code(new_code);
        
        // Attempt to compile the new instrumented code
        Error reload_err = script->reload(true);
        
        if (reload_err != OK) {
            Logger::error("Godot Engine refused to parse instrumented script: " + path + " (Error Code: " + String::num_int64(reload_err) + ")");
            script->set_source_code(original_code);
            script->reload(true);
            failed_count++;
            continue;
        }

        instrumented_count++;
    }

    Logger::info("Saving static coverage metadata...");
    api->save_static_metadata();
    
    Logger::info("--- Instrumentation Complete ---");
    Logger::info("Scripts Patched Successfully: " + String::num_int64(instrumented_count));
    Logger::info("Scripts Ignored (including Autoloads): " + String::num_int64(ignored_count));
    if (failed_count > 0) {
        Logger::error("Scripts Failed: " + String::num_int64(failed_count));
    }
}

} // namespace godot