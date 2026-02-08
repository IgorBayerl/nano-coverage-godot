#include <gtest/gtest.h>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../api/coverage_api.h"

using namespace godot;

TEST(CoverageApiTest, ContractTest) {
    // Instantiate API
    Ref<CoverageApi> api;
    api.instantiate();

    ASSERT_TRUE(api.is_valid());

    // Test instrument_project
    Dictionary instr_opts;
    Dictionary instr_result = api->instrument_project(instr_opts);

    // Verify success immediately
    if (instr_result.has("error")) {
        FAIL() << "Instrumentation failed: " << String(instr_result["error"]).utf8().get_data();
    }
    
    ASSERT_TRUE(instr_result.has("output_path")) << "instrument_project missing output_path";
    String output_path = instr_result["output_path"];

    // Test run_instrumented_project
    Dictionary run_opts;
    run_opts["output_path"] = output_path;
    run_opts["workspace_id"] = "test_workspace";
    run_opts["blocking"] = false;
    run_opts["dry_run"] = true; 

    Dictionary run_result = api->run_instrumented_project(run_opts);
    EXPECT_TRUE(run_result.has("args"));
    EXPECT_TRUE(run_result.has("run_id"));
    EXPECT_TRUE(run_result.has("expected_output_file"));

    // Test generate_coverage_report
    // Must run before clearing data, as it requires coverage.meta
    Dictionary report_opts;
    report_opts["workspace_id"] = "test_workspace";
    Dictionary report_result = api->generate_coverage_report(report_opts);
    
    if (report_result.has("error")) {
         FAIL() << "Report generation failed: " << String(report_result["error"]).utf8().get_data();
    }
    
    EXPECT_TRUE(report_result.has("status"));
    EXPECT_EQ(String(report_result["status"]), "ok");

    // Test clear_coverage_data
    // This deletes coverage.meta and .covdata files
    Dictionary clear_opts;
    clear_opts["workspace_id"] = "test_workspace";
    Dictionary clear_result = api->clear_coverage_data(clear_opts);
    EXPECT_TRUE(clear_result.has("status"));
    EXPECT_EQ(String(clear_result["status"]), "ok");
}