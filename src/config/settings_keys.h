#pragma once

namespace godot {

struct SettingsKeys {
    static constexpr const char* DATA_STORE_DIR = "nano_coverage/general/data_store_dir";
    static constexpr const char* REPORT_DIR = "nano_coverage/general/report_dir";
    static constexpr const char* IGNORE_PATHS = "nano_coverage/general/ignore_paths";
    static constexpr const char* IGNORE_ADDONS = "nano_coverage/general/ignore_addons";
    static constexpr const char* REPORT_LCOV_FILENAME = "nano_coverage/general/report_lcov_filename";

    // UI settings
    static constexpr const char* UI_SHOW_RUN_INSTRUMENTED = "nano_coverage/ui/show_run_instrumented";
    static constexpr const char* UI_SHOW_GENERATE_REPORT = "nano_coverage/ui/show_generate_report";
    static constexpr const char* UI_SHOW_CLEAR_DATA = "nano_coverage/ui/show_clear_data";
};

} // namespace godot
