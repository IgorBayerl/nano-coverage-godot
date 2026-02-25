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
    property_info["type"] = p_type;
    property_info["hint"] = p_hint;
    property_info["hint_string"] = p_hint_string;
    
    ps->add_property_info(property_info);
    ps->set_as_basic(p_name, true);
}

void SettingsGateway::register_settings() {
    _register_setting(SettingsKeys::DATA_STORE_DIR, "res://coverage-data", Variant::STRING, PROPERTY_HINT_GLOBAL_DIR);
    _register_setting(SettingsKeys::REPORT_DIR, "res://coverage-report", Variant::STRING, PROPERTY_HINT_GLOBAL_DIR);
    
    Array default_ignore;
    default_ignore.push_back("res://addons/nano_coverage_godot/**");
    default_ignore.push_back("res://addons/gdUnit4/**");
    default_ignore.push_back("**/*_test.gd"); // Example: ignore any file ending in _test.gd recursively
    _register_setting(SettingsKeys::IGNORE_PATHS, default_ignore, Variant::ARRAY);

    _register_setting(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info", Variant::STRING);
    _register_setting(SettingsKeys::USE_ABSOLUTE_PATHS, false, Variant::BOOL);
}

CoverageSettings SettingsGateway::load() {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    CoverageSettings settings;

    auto get_safe = [&](const String& path, const Variant& default_val) -> Variant {
        if (ps->has_setting(path)) return ps->get_setting(path);
        return default_val;
    };

    settings.data_store_dir = get_safe(SettingsKeys::DATA_STORE_DIR, "res://coverage-data");
    settings.report_dir = get_safe(SettingsKeys::REPORT_DIR, "res://coverage-report");
    settings.ignore_paths = get_safe(SettingsKeys::IGNORE_PATHS, Array());
    settings.report_lcov_filename = get_safe(SettingsKeys::REPORT_LCOV_FILENAME, "lcov.info");
    settings.use_absolute_paths = get_safe(SettingsKeys::USE_ABSOLUTE_PATHS, false);

    return settings;
}

} // namespace godot
