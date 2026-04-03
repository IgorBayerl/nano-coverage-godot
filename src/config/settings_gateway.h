#pragma once
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {

struct CoverageSettings {
    // General
    String data_store_dir;
    String report_dir;
    PackedStringArray ignore_paths;
    bool ignore_addons;
    String report_lcov_filename;
    bool watch_lcov_file = true;
    bool auto_generate_report = true;

    // UI
    bool ui_show_run_instrumented = true;
    bool ui_show_generate_report = true;
    bool ui_show_clear_data = true;
    bool ui_show_coverage_panel = true;
};

class SettingsGateway {
public:
    static void register_settings();
    static CoverageSettings load();
};

} // namespace godot
