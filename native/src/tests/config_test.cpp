#include <gtest/gtest.h>
#include "../config/settings_keys.h"
#include <cstring>
#include <string>

using namespace godot;

TEST(SettingsKeysTest, ConstantsAreDefined) {
    // Basic check to ensure the header is linkable and constants are not empty
    EXPECT_GT(std::strlen(SettingsKeys::TEMP_DIRECTORY), 0);
    EXPECT_EQ(std::string(SettingsKeys::TEMP_DIRECTORY), "nano_coverage/general/temp_directory");

    EXPECT_GT(std::strlen(SettingsKeys::PATHS_TEMP_DIR), 0);
    EXPECT_GT(std::strlen(SettingsKeys::PATHS_REPORT_DIR), 0);
    EXPECT_GT(std::strlen(SettingsKeys::PATHS_DATA_STORE_DIR), 0);
    
    EXPECT_GT(std::strlen(SettingsKeys::REPORT_LCOV_FILENAME), 0);
    EXPECT_GT(std::strlen(SettingsKeys::REPORT_USE_ABSOLUTE_SOURCE_PATHS), 0);

    EXPECT_GT(std::strlen(SettingsKeys::UI_SHOW_ALL_BUTTONS), 0);
    EXPECT_GT(std::strlen(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED_BUTTON), 0);
    EXPECT_GT(std::strlen(SettingsKeys::UI_SHOW_GENERATE_REPORT_BUTTON), 0);
    EXPECT_GT(std::strlen(SettingsKeys::UI_SHOW_CLEAR_DATA_BUTTON), 0);
}
