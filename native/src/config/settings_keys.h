#ifndef SETTINGS_KEYS_H
#define SETTINGS_KEYS_H

namespace godot {

struct SettingsKeys {
    // Current keys
    static constexpr const char* TEMP_DIRECTORY = "nano_coverage/general/temp_directory";

    // Future keys
    static constexpr const char* PATHS_TEMP_DIR = "nano_coverage/paths/temp_dir";
    static constexpr const char* PATHS_REPORT_DIR = "nano_coverage/paths/report_dir";
    static constexpr const char* PATHS_DATA_STORE_DIR = "nano_coverage/paths/data_store_dir";

    static constexpr const char* REPORT_LCOV_FILENAME = "nano_coverage/report/lcov_filename";
    static constexpr const char* REPORT_USE_ABSOLUTE_SOURCE_PATHS = "nano_coverage/report/use_absolute_source_paths";

    static constexpr const char* UI_SHOW_ALL_BUTTONS = "nano_coverage/ui/show_all_buttons";
    static constexpr const char* UI_SHOW_RUN_INSTRUMENTED_BUTTON = "nano_coverage/ui/show_run_instrumented_button";
    static constexpr const char* UI_SHOW_GENERATE_REPORT_BUTTON = "nano_coverage/ui/show_generate_report_button";
    static constexpr const char* UI_SHOW_CLEAR_DATA_BUTTON = "nano_coverage/ui/show_clear_data_button";
};

} // namespace godot

#endif // SETTINGS_KEYS_H
