#include <gtest/gtest.h>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "report_generator.h"
#include "../config/settings_keys.h"
#include "../config/settings_gateway.h"
#include "lcov_writer.h"
#include "../data/persistence.h"
#include "../testing/test_utils.h"

using namespace godot;

TEST(ReportGeneratorTest, MergesMetadataAndCovDataCorrectly) {
    String temp_dir = "res://temp_report_generator_test";

    // Clean before test
    TestUtils::clean_dir(temp_dir);
    
    // Override settings so the ReportGenerator looks in our temp directory
    SettingsOverride s1(SettingsKeys::DATA_STORE_DIR, temp_dir);
    SettingsOverride s2(SettingsKeys::REPORT_DIR, temp_dir);

    // Create the directory
    DirAccess::make_dir_recursive_absolute(temp_dir);
    DirAccess::make_dir_recursive_absolute(temp_dir + "/default/runs");

    // 1. Create a mock coverage.meta using Persistence
    CoverageMetadata meta;
    meta["res://some_script.gd"] = {5, 6, 7, 10}; // 4 coverable lines
    
    String global_temp_dir = ProjectSettings::get_singleton()->globalize_path(temp_dir);
    String meta_path = global_temp_dir.path_join("coverage.meta");
    Persistence::save_metadata(meta_path.utf8().get_data(), meta);

    // 2. Create a mock .covdata file using Persistence
    CoverageData covdata;
    covdata["res://some_script.gd"] = {{5, 2}, {7, 1}}; // Hits on line 5 (2 times) and line 7 (1 time). Lines 6 and 10 have 0 hits.
    
    String cov_file_path = global_temp_dir.path_join("default/runs/run1.covdata");
    Persistence::append_execution_data(cov_file_path.utf8().get_data(), covdata);

    // 3. Run Generator
    Dictionary options;
    options["workspace_id"] = "default";
    Dictionary result = ReportGenerator::generate(options);

    ASSERT_STREQ(String(result["status"]).utf8().get_data(), "ok");

    // 4. Verify output LCOV string format
    String lcov_path = temp_dir + "/lcov.info";
    ASSERT_TRUE(FileAccess::file_exists(lcov_path));
    
    Ref<FileAccess> file = FileAccess::open(lcov_path, FileAccess::READ);
    String content = file->get_as_text();
    file->close();
    
    // The key validation: lines without hits (e.g. 6 and 10) should explicitly report "DA:6,0" and "DA:10,0" in the final output
    EXPECT_TRUE(content.contains("DA:5,2"));
    EXPECT_TRUE(content.contains("DA:6,0"));
    EXPECT_TRUE(content.contains("DA:7,1"));
    EXPECT_TRUE(content.contains("DA:10,0"));
    
    //LF: 4 (Total Coverable), LH: 2 (Total Hit)
    EXPECT_TRUE(content.contains("LF:4"));
    EXPECT_TRUE(content.contains("LH:2"));

    // Cleanup
    TestUtils::clean_dir(temp_dir);
}
