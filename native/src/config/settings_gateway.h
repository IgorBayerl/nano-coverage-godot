#ifndef SETTINGS_GATEWAY_H
#define SETTINGS_GATEWAY_H

#include <godot_cpp/variant/string.hpp>

namespace godot {

struct CoverageSettings {
    // Current keys
    String temp_directory;

    // Future keys
    String paths_temp_dir;
    String paths_report_dir;
    String paths_data_store_dir;

    String report_lcov_filename;
    bool report_use_absolute_source_paths;

    bool ui_show_all_buttons;
    bool ui_show_run_instrumented_button;
    bool ui_show_generate_report_button;
    bool ui_show_clear_data_button;
};

class SettingsGateway {
public:
    static void register_settings();
    static CoverageSettings load();
};

} // namespace godot

#endif // SETTINGS_GATEWAY_H
