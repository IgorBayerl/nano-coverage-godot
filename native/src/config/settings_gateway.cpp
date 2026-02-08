#include "settings_gateway.h"
#include "settings_keys.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void SettingsGateway::register_settings() {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    // Helper lambda to register a setting
    auto register_key = [&](const String& path, const Variant& default_value, Variant::Type type, PropertyHint hint = PROPERTY_HINT_NONE, const String& hint_string = "") {
        if (!ps->has_setting(path)) {
            ps->set_setting(path, default_value);
        }
        ps->set_initial_value(path, default_value);
        
        Dictionary property_info;
        property_info["name"] = path;
        property_info["type"] = type;
        property_info["hint"] = hint;
        property_info["hint_string"] = hint_string;
        ps->add_property_info(property_info);
        ps->set_as_basic(path, true);
    };

    // Current keys
    register_key(SettingsKeys::TEMP_DIRECTORY, "", Variant::STRING, PROPERTY_HINT_GLOBAL_DIR, "Folder to store the instrumented project");

    // Future keys - Paths
    register_key(SettingsKeys::PATHS_TEMP_DIR, "", Variant::STRING, PROPERTY_HINT_GLOBAL_DIR, "Same as temp_directory (Transition)");
    register_key(SettingsKeys::PATHS_REPORT_DIR, "res://coverage_report", Variant::STRING, PROPERTY_HINT_GLOBAL_DIR);
    register_key(SettingsKeys::PATHS_DATA_STORE_DIR, "res://coverage_data", Variant::STRING, PROPERTY_HINT_GLOBAL_DIR);

    // Future keys - Report
    register_key(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info", Variant::STRING);
    register_key(SettingsKeys::REPORT_USE_ABSOLUTE_SOURCE_PATHS, false, Variant::BOOL);

    // Future keys - UI
    register_key(SettingsKeys::UI_SHOW_ALL_BUTTONS, true, Variant::BOOL);
    register_key(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED_BUTTON, true, Variant::BOOL);
    register_key(SettingsKeys::UI_SHOW_GENERATE_REPORT_BUTTON, true, Variant::BOOL);
    register_key(SettingsKeys::UI_SHOW_CLEAR_DATA_BUTTON, true, Variant::BOOL);
}

CoverageSettings SettingsGateway::load() {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    CoverageSettings settings;

    // Helper implementation to safely get a setting
    auto get_safe = [&](const String& path, const Variant& default_val) -> Variant {
        if (ps->has_setting(path)) {
            return ps->get_setting(path);
        }
        // Fallback: Try getting it anyway, in case has_setting is false but it exists (e.g. dynamic override)
        Variant val = ps->get_setting(path);
        if (val.get_type() != Variant::NIL) {
            return val;
        }
        return default_val;
    };

    settings.temp_directory = get_safe(SettingsKeys::TEMP_DIRECTORY, "");
    
    // For paths, if we get an empty string, we should probably use default if default is non-empty. 
    // This protects against "set to empty" pollution or user error.
    
    auto get_path = [&](const String& key, const String& def) -> String {
        String val = get_safe(key, def);
        if (val.is_empty() && !def.is_empty()) return def;
        return val;
    };

    settings.paths_temp_dir = get_path(SettingsKeys::PATHS_TEMP_DIR, "");
    // Default for report dir is res://coverage_report. If retrieval gives "", use default.
    settings.paths_report_dir = get_path(SettingsKeys::PATHS_REPORT_DIR, "res://coverage_report");
    settings.paths_data_store_dir = get_path(SettingsKeys::PATHS_DATA_STORE_DIR, "res://coverage_data");

    settings.report_lcov_filename = get_safe(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info");
    settings.report_use_absolute_source_paths = get_safe(SettingsKeys::REPORT_USE_ABSOLUTE_SOURCE_PATHS, false);

    settings.ui_show_all_buttons = get_safe(SettingsKeys::UI_SHOW_ALL_BUTTONS, true);
    settings.ui_show_run_instrumented_button = get_safe(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED_BUTTON, true);
    settings.ui_show_generate_report_button = get_safe(SettingsKeys::UI_SHOW_GENERATE_REPORT_BUTTON, true);
    settings.ui_show_clear_data_button = get_safe(SettingsKeys::UI_SHOW_CLEAR_DATA_BUTTON, true);

    return settings;
}

} // namespace godot
