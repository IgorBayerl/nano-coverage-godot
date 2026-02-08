#include <gtest/gtest.h>
#include "../config/settings_gateway.h"
#include "../config/settings_keys.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/string.hpp>
#include <filesystem>
#include "test_utils.h"

using namespace godot;

TEST(SettingsGatewayTest, RegisterAndDefaultValues) {
    // Ensure we are reading clean defaults by overriding potentially dirty ones from other tests
    // (Though with the fixes above, they should be clean, but this makes this test self-sufficient)
    SettingsGateway::register_settings();
    CoverageSettings settings = SettingsGateway::load();

    EXPECT_EQ(settings.temp_directory, "");
    EXPECT_EQ(settings.paths_report_dir, "res://coverage_report");
    EXPECT_EQ(settings.paths_data_store_dir, "res://coverage_data");
    EXPECT_TRUE(settings.ui_show_all_buttons);
}

TEST(SettingsGatewayTest, LoadReadsValuesFromProjectSettings) {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    
    // Setup overrides using RAII
    SettingsOverride s1(SettingsKeys::TEMP_DIRECTORY, "custom/temp");
    SettingsOverride s2(SettingsKeys::PATHS_REPORT_DIR, "custom/report");
    SettingsOverride s3(SettingsKeys::PATHS_DATA_STORE_DIR, "custom/data");
    SettingsOverride s4(SettingsKeys::REPORT_LCOV_FILENAME, "custom.info");
    SettingsOverride s5(SettingsKeys::REPORT_USE_ABSOLUTE_SOURCE_PATHS, true);
    SettingsOverride s6(SettingsKeys::UI_SHOW_ALL_BUTTONS, false);

    // Load settings
    CoverageSettings settings = SettingsGateway::load();

    // Assert
    EXPECT_EQ(settings.temp_directory, "custom/temp");
    EXPECT_EQ(settings.paths_report_dir, "custom/report");
    EXPECT_EQ(settings.paths_data_store_dir, "custom/data");
    EXPECT_EQ(settings.report_lcov_filename, "custom.info");
    EXPECT_TRUE(settings.report_use_absolute_source_paths);
    EXPECT_FALSE(settings.ui_show_all_buttons);
    
    // Destructors restore original values automatically
}

TEST(SettingsGatewayTest, RegisterSetsUpKeysAndTypes) {
    SettingsGateway::register_settings();
    ProjectSettings* ps = ProjectSettings::get_singleton();
    
    EXPECT_TRUE(ps->has_setting(SettingsKeys::TEMP_DIRECTORY));
    EXPECT_EQ(ps->get_setting(SettingsKeys::TEMP_DIRECTORY).get_type(), Variant::STRING);
    EXPECT_TRUE(ps->has_setting(SettingsKeys::PATHS_REPORT_DIR));
}