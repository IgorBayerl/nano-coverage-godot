#include <cstring>
#include <gtest/gtest.h>
#include <string>

#include "settings_keys.h"

using namespace godot;

TEST(SettingsKeysTest, ConstantsAreDefined) {
    EXPECT_GT(std::strlen(SettingsKeys::DATA_STORE_DIR), 0);
    EXPECT_GT(std::strlen(SettingsKeys::REPORT_DIR), 0);
    EXPECT_GT(std::strlen(SettingsKeys::IGNORE_PATHS), 0);
    EXPECT_GT(std::strlen(SettingsKeys::IGNORE_ADDONS), 0);
    EXPECT_GT(std::strlen(SettingsKeys::REPORT_LCOV_FILENAME), 0);
}
