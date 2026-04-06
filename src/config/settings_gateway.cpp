#include "settings_gateway.h"
#include "settings_keys.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

static void _register_setting(const String& p_name, const Variant& p_default, Variant::Type p_type, PropertyHint p_hint = PROPERTY_HINT_NONE, const String& p_hint_string = "") {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    if (!ps->has_setting(p_name)) {
        ps->set_setting(p_name, p_default);
    }
    ps->set_initial_value(p_name, p_default);

    Dictionary property_info;
    property_info["name"] = p_name;
    property_info["type"] = (int)p_type;
    property_info["hint"] = (int)p_hint;
    property_info["hint_string"] = p_hint_string;

    ps->add_property_info(property_info);
    ps->set_as_basic(p_name, true);
}

void SettingsGateway::register_settings() {
    // General
    _register_setting(SettingsKeys::DATA_STORE_DIR, "res://coverage-data", Variant::STRING, PROPERTY_HINT_DIR);
    _register_setting(SettingsKeys::REPORT_DIR, "res://coverage-report", Variant::STRING, PROPERTY_HINT_DIR);

    PackedStringArray default_ignore;
    default_ignore.push_back("**/*_test.gd");
    _register_setting(SettingsKeys::IGNORE_PATHS, default_ignore, Variant::PACKED_STRING_ARRAY);

    _register_setting(SettingsKeys::IGNORE_ADDONS, true, Variant::BOOL);
    _register_setting(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info", Variant::STRING);
    _register_setting(SettingsKeys::WATCH_LCOV_FILE, true, Variant::BOOL);
    _register_setting(SettingsKeys::AUTO_GENERATE_REPORT, true, Variant::BOOL);
    _register_setting(SettingsKeys::BACKUP_DIR, "res://.nano_coverage", Variant::STRING, PROPERTY_HINT_DIR);

    // UI
    _register_setting(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED, false, Variant::BOOL);
    _register_setting(SettingsKeys::UI_SHOW_GENERATE_REPORT, false, Variant::BOOL);
    _register_setting(SettingsKeys::UI_SHOW_CLEAR_DATA, false, Variant::BOOL);
    _register_setting(SettingsKeys::UI_SHOW_COVERAGE_PANEL, true, Variant::BOOL);
    
    // UI Thresholds (Range 0.0 to 100.0, step 0.1)
    _register_setting(SettingsKeys::THRESHOLD_HIGH, 80.0f, Variant::FLOAT, PROPERTY_HINT_RANGE, "0.0,100.0,0.1");
    _register_setting(SettingsKeys::THRESHOLD_MEDIUM, 50.0f, Variant::FLOAT, PROPERTY_HINT_RANGE, "0.0,100.0,0.1");
}

CoverageSettings SettingsGateway::load() {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    CoverageSettings settings;

    auto get_safe = [&](const String& path, const Variant& default_val) -> Variant {
        if (ps->has_setting(path)) return ps->get_setting(path);
        return default_val;
    };

    // General
    settings.data_store_dir = get_safe(SettingsKeys::DATA_STORE_DIR, "coverage-data");
    settings.report_dir = get_safe(SettingsKeys::REPORT_DIR, "coverage-report");
    settings.ignore_paths = get_safe(SettingsKeys::IGNORE_PATHS, PackedStringArray());
    settings.ignore_addons = get_safe(SettingsKeys::IGNORE_ADDONS, true);
    settings.report_lcov_filename = get_safe(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info");
    settings.watch_lcov_file = get_safe(SettingsKeys::WATCH_LCOV_FILE, true);
    settings.auto_generate_report = get_safe(SettingsKeys::AUTO_GENERATE_REPORT, true);
    settings.backup_dir = get_safe(SettingsKeys::BACKUP_DIR, "res://.nano_coverage");

    // UI
    settings.ui_show_run_instrumented = get_safe(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED, false);
    settings.ui_show_generate_report = get_safe(SettingsKeys::UI_SHOW_GENERATE_REPORT, false);
    settings.ui_show_clear_data = get_safe(SettingsKeys::UI_SHOW_CLEAR_DATA, false);
    settings.ui_show_coverage_panel = get_safe(SettingsKeys::UI_SHOW_COVERAGE_PANEL, true);
    
    // UI Thresholds
    settings.threshold_high = get_safe(SettingsKeys::THRESHOLD_HIGH, 80.0f);
    settings.threshold_medium = get_safe(SettingsKeys::THRESHOLD_MEDIUM, 50.0f);

    return settings;
}

} // namespace godot