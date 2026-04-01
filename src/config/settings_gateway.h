#pragma once
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {

struct CoverageSettings {
    String data_store_dir;
    String report_dir;
    PackedStringArray ignore_paths;
    bool ignore_addons;
    String report_lcov_filename;

    // UI settings
    bool ui_show_run_instrumented = true;
    bool ui_show_generate_report = true;
    bool ui_show_clear_data = true;
};

class SettingsGateway {
public:
    static void register_settings();
    static CoverageSettings load();
};

} // namespace godot
