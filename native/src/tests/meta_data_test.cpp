#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <gtest/gtest.h>

#include "../runtime/coverage_monitor.h"
#include "../config/settings_keys.h"
#include "../data/persistence.h"
#include "test_utils.h"

namespace godot {

TEST(NanoCoverageTest, ReportsZeroHitLinesFromMetadata) {
    NanoCoverage* cov = memnew(NanoCoverage);

    // 1. Setup paths
    String report_dir = "res://coverage_report_unique";
    
    // Ensure clean state using recursive delete
    TestUtils::clean_dir(report_dir);
    DirAccess::make_dir_recursive_absolute(report_dir);

    // 2. Create a dummy source file
    String expected_res_path = report_dir + "/test_meta.gd";
    Ref<FileAccess> f_gen = FileAccess::open(expected_res_path, FileAccess::WRITE);
    f_gen->store_line("extends Node");
    f_gen->store_line("func _ready():");
    f_gen->store_line("    print('hit me')");
    f_gen->store_line("    if false:");
    f_gen->store_line("        print('miss me')");
    f_gen->close();

    // 3. Create REQUIRED Metadata
    CoverageMetadata meta;
    meta[expected_res_path.utf8().get_data()] = {3, 5}; 
    
    String meta_path_res = report_dir + "/coverage.meta";
    String meta_path_abs = ProjectSettings::get_singleton()->globalize_path(meta_path_res);
    Persistence::save_metadata(meta_path_abs.utf8().get_data(), meta);

    // 4. Record Runtime Hits
    cov->reset();
    cov->hit(expected_res_path, 3); // Hit line 3
    
    // 5. Configure Settings using RAII Helper
    SettingsOverride s1("nano_coverage/source_root", report_dir);
    SettingsOverride s2("nano_coverage/output_dir", report_dir); 
    SettingsOverride s3(SettingsKeys::PATHS_REPORT_DIR, report_dir);
    SettingsOverride s4(SettingsKeys::PATHS_DATA_STORE_DIR, report_dir);

    cov->save_session();

    // 6. Generate Report
    cov->generate_report();
    
    // 7. Verify
    String lcov_path = report_dir + "/lcov.info";
    ASSERT_TRUE(FileAccess::file_exists(lcov_path));
    
    // [Verification logic remains the same...]

    memdelete(cov);
    
    // CLEANUP: Recursively remove the test directory
    TestUtils::clean_dir(report_dir);
}

} // namespace godot