#pragma once

namespace godot {

struct SettingsKeys {
    static constexpr const char* DATA_STORE_DIR = "nano_coverage/general/data_store_dir";
    static constexpr const char* REPORT_DIR = "nano_coverage/general/report_dir";
    static constexpr const char* IGNORE_PATHS = "nano_coverage/general/ignore_paths";
    static constexpr const char* IGNORE_ADDONS = "nano_coverage/general/ignore_addons";
    static constexpr const char* REPORT_LCOV_FILENAME = "nano_coverage/general/report_lcov_filename";
};

} // namespace godot
