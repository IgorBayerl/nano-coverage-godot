#include "project_bootstrapper.h"

#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "../api/coverage_api.h"
#include "../config/settings_gateway.h"
#include "../utils/logger.h"
#include "script_scanner.h"

namespace godot {

void ProjectBootstrapper::_bind_methods() {
    ClassDB::bind_method(D_METHOD("instrument_all_scripts"), &ProjectBootstrapper::instrument_all_scripts);
}

void ProjectBootstrapper::instrument_all_scripts() {
    Logger::info("--- Starting Memory Instrumentation ---");

    CoverageSettings settings = SettingsGateway::load();
    Array files = ScriptScanner::scan_project(settings);
    Logger::info("Total GDScript files found to instrument: " + String::num_int64(files.size()));

    // --- AUTOLOAD DETECTION ---
    // Autoloads are already loaded (and possibly running) when this executes,
    // so hot-patching them would wipe their state. They are skipped here;
    // use disk instrumentation to cover them.
    Array autoload_paths;
    TypedArray<Dictionary> props = ProjectSettings::get_singleton()->get_property_list();
    for (int i = 0; i < props.size(); i++) {
        Dictionary prop = props[i];
        String name = prop["name"];
        if (name.begins_with("autoload/")) {
            String path = ProjectSettings::get_singleton()->get_setting(name);
            // Autoloads are formatted as "*res://path/to/script.gd" when enabled
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
            Logger::error("Godot Engine refused to parse instrumented script: " + path +
                          " (Error Code: " + String::num_int64(reload_err) + ")");
            script->set_source_code(original_code);
            script->reload(true);
            failed_count++;
            continue;
        }

        instrumented_count++;
    }

    Logger::info("Saving static coverage metadata...");
    api->save_static_metadata();

    Logger::info("--- Memory Instrumentation Complete ---");
    Logger::info("Scripts Patched Successfully: " + String::num_int64(instrumented_count));
    Logger::info("Scripts Ignored (including Autoloads): " + String::num_int64(ignored_count));
    if (failed_count > 0) {
        Logger::error("Scripts Failed: " + String::num_int64(failed_count));
    }
}

}  // namespace godot
