#include "settings_gateway.h"

#include <filesystem>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/string.hpp>
#include <gtest/gtest.h>

#include "settings_keys.h"
#include "testing/test_utils.h"

using namespace godot;

TEST(SettingsGatewayTest, RegisterAndDefaultValues) {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    ps->clear(SettingsKeys::DATA_STORE_DIR);
    ps->clear(SettingsKeys::REPORT_DIR);
    ps->clear(SettingsKeys::IGNORE_PATHS);
    ps->clear(SettingsKeys::IGNORE_ADDONS);
    ps->clear(SettingsKeys::REPORT_LCOV_FILENAME);

    SettingsGateway::register_settings();
    CoverageSettings settings = SettingsGateway::load();

    EXPECT_EQ(settings.data_store_dir, "res://coverage-data");
    EXPECT_EQ(settings.report_dir, "res://coverage-report");
    EXPECT_EQ(settings.report_lcov_filename, "lcov.info");
    EXPECT_TRUE(settings.ignore_addons);
}

TEST(SettingsGatewayTest, LoadReadsValuesFromProjectSettings) {
    ProjectSettings* ps = ProjectSettings::get_singleton();

    // Setup overrides using RAII
    PackedStringArray custom_ignores;
    custom_ignores.push_back("test/**");
    SettingsOverride s1(SettingsKeys::DATA_STORE_DIR, "custom/data");
    SettingsOverride s2(SettingsKeys::REPORT_DIR, "custom/report");
    SettingsOverride s3(SettingsKeys::IGNORE_PATHS, custom_ignores);
    SettingsOverride s4(SettingsKeys::REPORT_LCOV_FILENAME, "custom.info");
    SettingsOverride s6(SettingsKeys::IGNORE_ADDONS, false);

    // Load settings
    CoverageSettings settings = SettingsGateway::load();

    // Assert
    EXPECT_EQ(settings.data_store_dir, "custom/data");
    EXPECT_EQ(settings.report_dir, "custom/report");
    EXPECT_EQ(settings.ignore_paths, custom_ignores);
    EXPECT_EQ(settings.report_lcov_filename, "custom.info");
    EXPECT_FALSE(settings.ignore_addons);

    // Destructors restore original values automatically
}

TEST(SettingsGatewayTest, RegisterSetsUpKeysAndTypes) {
    SettingsGateway::register_settings();
    ProjectSettings* ps = ProjectSettings::get_singleton();

    EXPECT_TRUE(ps->has_setting(SettingsKeys::DATA_STORE_DIR));
    EXPECT_EQ(ps->get_setting(SettingsKeys::DATA_STORE_DIR).get_type(), Variant::STRING);
    EXPECT_TRUE(ps->has_setting(SettingsKeys::REPORT_DIR));
    EXPECT_TRUE(ps->has_setting(SettingsKeys::IGNORE_PATHS));
    EXPECT_EQ(ps->get_setting(SettingsKeys::IGNORE_PATHS).get_type(), Variant::PACKED_STRING_ARRAY);
    EXPECT_TRUE(ps->has_setting(SettingsKeys::IGNORE_ADDONS));

    // UI settings
    EXPECT_TRUE(ps->has_setting(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED));
    EXPECT_TRUE(ps->has_setting(SettingsKeys::UI_SHOW_GENERATE_REPORT));
    EXPECT_TRUE(ps->has_setting(SettingsKeys::UI_SHOW_CLEAR_DATA));
}

TEST(SettingsGatewayTest, UIButtonVisibilityDefaults) {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    ps->clear(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED);
    ps->clear(SettingsKeys::UI_SHOW_GENERATE_REPORT);
    ps->clear(SettingsKeys::UI_SHOW_CLEAR_DATA);

    SettingsGateway::register_settings();
    CoverageSettings s = SettingsGateway::load();

    EXPECT_TRUE(s.ui_show_run_instrumented);
    EXPECT_TRUE(s.ui_show_generate_report);
    EXPECT_TRUE(s.ui_show_clear_data);
}

TEST(SettingsGatewayTest, UIButtonVisibilityOverrides) {
    SettingsOverride o1(SettingsKeys::UI_SHOW_RUN_INSTRUMENTED, false);
    SettingsOverride o2(SettingsKeys::UI_SHOW_GENERATE_REPORT, false);
    SettingsOverride o3(SettingsKeys::UI_SHOW_CLEAR_DATA, true);

    CoverageSettings s = SettingsGateway::load();
    EXPECT_FALSE(s.ui_show_run_instrumented);
    EXPECT_FALSE(s.ui_show_generate_report);
    EXPECT_TRUE(s.ui_show_clear_data);
}