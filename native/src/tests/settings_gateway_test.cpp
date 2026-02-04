#include <gtest/gtest.h>
#include "../config/settings_gateway.h"
#include "../config/settings_keys.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/string.hpp>
#include <filesystem>

using namespace godot;

TEST(SettingsGatewayTest, RegisterAndDefaultValues) {
    // Register settings
    SettingsGateway::register_settings();

    // Load settings
    CoverageSettings settings = SettingsGateway::load();

    // Assert default values
    EXPECT_EQ(settings.temp_directory, "");
    
    EXPECT_EQ(settings.paths_temp_dir, "");
    EXPECT_EQ(settings.paths_report_dir, "res://coverage_report");
    EXPECT_EQ(settings.paths_data_store_dir, "res://coverage_data");

    EXPECT_EQ(settings.report_lcov_filename, "lcov.info");
    EXPECT_FALSE(settings.report_use_absolute_source_paths);

    EXPECT_TRUE(settings.ui_show_all_buttons);
    EXPECT_TRUE(settings.ui_show_run_instrumented_button);
    EXPECT_TRUE(settings.ui_show_generate_report_button);
    EXPECT_TRUE(settings.ui_show_clear_data_button);
}

TEST(SettingsGatewayTest, LoadReadsValuesFromProjectSettings) {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    // Save original state for critical settings
    Variant original_temp_dir;
    if (ps->has_setting(SettingsKeys::TEMP_DIRECTORY)) {
        original_temp_dir = ps->get_setting(SettingsKeys::TEMP_DIRECTORY);
    }

    // Setup known values in ProjectSettings to test mapping
    
    std::filesystem::path temp_path_fs = std::filesystem::temp_directory_path() / "nc_test_safe";
    String test_path(temp_path_fs.string().c_str());

    ps->set_setting(SettingsKeys::TEMP_DIRECTORY, test_path);
    
    ps->set_setting(SettingsKeys::PATHS_TEMP_DIR, "custom/paths/temp");
    ps->set_setting(SettingsKeys::PATHS_REPORT_DIR, "custom/paths/report");
    ps->set_setting(SettingsKeys::PATHS_DATA_STORE_DIR, "custom/paths/data");

    ps->set_setting(SettingsKeys::REPORT_LCOV_FILENAME, "custom_lcov.info");
    ps->set_setting(SettingsKeys::REPORT_USE_ABSOLUTE_SOURCE_PATHS, true);

    ps->set_setting(SettingsKeys::UI_SHOW_ALL_BUTTONS, false);
    ps->set_setting(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED_BUTTON, false);
    ps->set_setting(SettingsKeys::UI_SHOW_GENERATE_REPORT_BUTTON, false);
    ps->set_setting(SettingsKeys::UI_SHOW_CLEAR_DATA_BUTTON, false);

    // Load settings
    CoverageSettings settings = SettingsGateway::load();

    // Assert that the struct was populated correctly
    EXPECT_EQ(settings.temp_directory, test_path);
    
    EXPECT_EQ(settings.paths_temp_dir, "custom/paths/temp");
    EXPECT_EQ(settings.paths_report_dir, "custom/paths/report");
    EXPECT_EQ(settings.paths_data_store_dir, "custom/paths/data");

    EXPECT_EQ(settings.report_lcov_filename, "custom_lcov.info");
    EXPECT_TRUE(settings.report_use_absolute_source_paths);

    EXPECT_FALSE(settings.ui_show_all_buttons);
    EXPECT_FALSE(settings.ui_show_run_instrumented_button);
    EXPECT_FALSE(settings.ui_show_generate_report_button);
    EXPECT_FALSE(settings.ui_show_clear_data_button);

    // Cleanup to avoid side-effects on other tests
    if (original_temp_dir.get_type() != Variant::NIL) {
        ps->set_setting(SettingsKeys::TEMP_DIRECTORY, original_temp_dir);
    } else {
        // If it wasn't set, we can set it to default or empty
        ps->set_setting(SettingsKeys::TEMP_DIRECTORY, "");
    }
}

TEST(SettingsGatewayTest, RegisterSetsUpKeysAndTypes) {
    // This test is safe as it registers (which calls add_property_info) but doesn't overwrite values if they exist
    // unless register_settings implementation forces it?
    // Implementation says: if (!ps->has_setting) ps->set_setting;
    // So it is safe.
    SettingsGateway::register_settings();
    ProjectSettings* ps = ProjectSettings::get_singleton();

    // Verify keys exist and have correct types
    
    EXPECT_TRUE(ps->has_setting(SettingsKeys::TEMP_DIRECTORY));
    EXPECT_EQ(ps->get_setting(SettingsKeys::TEMP_DIRECTORY).get_type(), Variant::STRING);
    
    EXPECT_TRUE(ps->has_setting(SettingsKeys::PATHS_REPORT_DIR));
    EXPECT_EQ(ps->get_setting(SettingsKeys::PATHS_REPORT_DIR).get_type(), Variant::STRING);
    
    EXPECT_TRUE(ps->has_setting(SettingsKeys::REPORT_USE_ABSOLUTE_SOURCE_PATHS));
    EXPECT_EQ(ps->get_setting(SettingsKeys::REPORT_USE_ABSOLUTE_SOURCE_PATHS).get_type(), Variant::BOOL);
}
