#pragma once

namespace godot {

struct SettingsKeys {
    static constexpr const char* DATA_STORE_DIR = "nano_coverage/data_store_dir";
    static constexpr const char* REPORT_DIR = "nano_coverage/report_dir";
    static constexpr const char* IGNORE_PATHS = "nano_coverage/ignore_paths";
    static constexpr const char* REPORT_LCOV_FILENAME = "nano_coverage/report_lcov_filename";
    static constexpr const char* USE_ABSOLUTE_PATHS = "nano_coverage/use_absolute_paths";
};

} // namespace godot
