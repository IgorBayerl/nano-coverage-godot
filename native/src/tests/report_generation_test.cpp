#include "test_main.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <gtest/gtest.h>

#include "../runtime/coverage_monitor.h"
#include "../config/settings_keys.h"
#include "../data/persistence.h"
#include "test_utils.h"

namespace godot {

TEST(NanoCoverageTest, GeneratesReportAndCreatesDirectories) {
    NanoCoverage* cov = memnew(NanoCoverage);
    
    // Setup paths
    String output_dir = "res://coverage_report_gen_test";
    String data_dir = "res://coverage_data_gen_test";
    
    TestUtils::clean_dir(output_dir);
    TestUtils::clean_dir(data_dir);
    DirAccess::make_dir_recursive_absolute(data_dir);
    
    // 1. Create Dummy Metadata
    String test_file = "res://test_report.gd";
    CoverageMetadata meta;
    meta[test_file.utf8().get_data()] = {5};
    
    String meta_path_abs = ProjectSettings::get_singleton()->globalize_path(data_dir + "/coverage.meta");
    Persistence::save_metadata(meta_path_abs.utf8().get_data(), meta);

    // 2. Setup Settings Overrides
    SettingsOverride s1(SettingsKeys::PATHS_REPORT_DIR, output_dir);
    SettingsOverride s2(SettingsKeys::PATHS_DATA_STORE_DIR, data_dir);
    SettingsOverride s3("nano_coverage/output_dir", data_dir);

    // 3. Record Hits & Save
    cov->reset();
    cov->hit(test_file, 5);
    cov->save_session();

    // 4. Generate
    cov->generate_report();

    // 5. Verify
    EXPECT_TRUE(DirAccess::dir_exists_absolute(output_dir)) << "Report directory was not created.";
    String output_file = output_dir + "/lcov.info";
    EXPECT_TRUE(FileAccess::file_exists(output_file)) << "Report file was not created.";

    // Cleanup
    memdelete(cov);
    TestUtils::clean_dir(output_dir);
    TestUtils::clean_dir(data_dir);
}

} // namespace godot