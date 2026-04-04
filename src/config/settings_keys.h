#pragma once

namespace godot {

struct SettingsKeys {
    // General
    static constexpr const char* DATA_STORE_DIR = "nano_coverage/general/data_store_dir";
    static constexpr const char* REPORT_DIR = "nano_coverage/general/report_dir";
    static constexpr const char* IGNORE_PATHS = "nano_coverage/general/ignore_paths";
    static constexpr const char* IGNORE_ADDONS = "nano_coverage/general/ignore_addons";
    static constexpr const char* REPORT_LCOV_FILENAME = "nano_coverage/general/report_lcov_filename";
    static constexpr const char* WATCH_LCOV_FILE = "nano_coverage/general/watch_lcov_file";
    static constexpr const char* AUTO_GENERATE_REPORT = "nano_coverage/general/auto_generate_report";

    // UI
    static constexpr const char* UI_SHOW_RUN_INSTRUMENTED = "nano_coverage/ui/show_run_instrumented";
    static constexpr const char* UI_SHOW_GENERATE_REPORT = "nano_coverage/ui/show_generate_report";
    static constexpr const char* UI_SHOW_CLEAR_DATA = "nano_coverage/ui/show_clear_data";
    static constexpr const char* UI_SHOW_COVERAGE_PANEL = "nano_coverage/ui/show_coverage_panel";
    
    // UI Thresholds (Grouped under "thresholds")
    static constexpr const char* THRESHOLD_HIGH = "nano_coverage/ui/thresholds/high_percent";
    static constexpr const char* THRESHOLD_MEDIUM = "nano_coverage/ui/thresholds/medium_percent";
};

} // namespace godot
