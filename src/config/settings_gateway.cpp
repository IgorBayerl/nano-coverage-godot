#include "settings_gateway.h"
#include "settings_keys.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

// Private helper to isolate Godot's required string-based Dictionary
static void _register_setting(const String& p_name, const Variant& p_default, Variant::Type p_type, PropertyHint p_hint = PROPERTY_HINT_NONE, const String& p_hint_string = "") {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    
    if (!ps->has_setting(p_name)) {
        ps->set_setting(p_name, p_default);
    }
    ps->set_initial_value(p_name, p_default);

    Dictionary property_info;
    property_info["name"] = p_name;
    property_info["type"] = (int)p_type; // Fixes Godot parsing the Dictionary 
    property_info["hint"] = (int)p_hint;
    property_info["hint_string"] = p_hint_string;
    
    ps->add_property_info(property_info);
    ps->set_as_basic(p_name, true);
}

	// "res://addons/gdUnit4/src/network",
	// "res://addons/gdUnit4/src/core/runners",
	// "res://addons/gdUnit4/src/ui",
	// "res://addons/gdUnit4/bin",
	// "res://addons/gdUnit4/src/core/hooks",
	
	// # --- GdUnit4 Tests ---
	// # We ignore all test files. They will still RUN, but they won't be instrumented.
	// # This avoids parser errors from "intentional failure" scripts and keeps the report focused on source code.
	// "res://addons/gdUnit4/test"

void SettingsGateway::register_settings() {
    _register_setting(SettingsKeys::DATA_STORE_DIR, "res://coverage-data", Variant::STRING, PROPERTY_HINT_DIR);
    _register_setting(SettingsKeys::REPORT_DIR, "res://coverage-report", Variant::STRING, PROPERTY_HINT_DIR);
    
    PackedStringArray default_ignore;
    default_ignore.push_back("**/*_test.gd"); // Explicitly ignore unit test gdscripts in the report
    _register_setting(SettingsKeys::IGNORE_PATHS, default_ignore, Variant::PACKED_STRING_ARRAY);

    _register_setting(SettingsKeys::IGNORE_ADDONS, true, Variant::BOOL);

    _register_setting(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info", Variant::STRING);

    // UI settings
    _register_setting(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED, true, Variant::BOOL);
    _register_setting(SettingsKeys::UI_SHOW_GENERATE_REPORT, true, Variant::BOOL);
    _register_setting(SettingsKeys::UI_SHOW_CLEAR_DATA, true, Variant::BOOL);
}

CoverageSettings SettingsGateway::load() {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    CoverageSettings settings;

    auto get_safe = [&](const String& path, const Variant& default_val) -> Variant {
        if (ps->has_setting(path)) return ps->get_setting(path);
        return default_val;
    };

    settings.data_store_dir = get_safe(SettingsKeys::DATA_STORE_DIR, "coverage-data");
    settings.report_dir = get_safe(SettingsKeys::REPORT_DIR, "coverage-report");
    settings.ignore_paths = get_safe(SettingsKeys::IGNORE_PATHS, PackedStringArray());
    settings.ignore_addons = get_safe(SettingsKeys::IGNORE_ADDONS, true);
    settings.report_lcov_filename = get_safe(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info");

    // UI settings
    settings.ui_show_run_instrumented = get_safe(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED, true);
    settings.ui_show_generate_report = get_safe(SettingsKeys::UI_SHOW_GENERATE_REPORT, true);
    settings.ui_show_clear_data = get_safe(SettingsKeys::UI_SHOW_CLEAR_DATA, true);

    return settings;
}

} // namespace godot
