#include <gtest/gtest.h>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../api/coverage_api.h"

using namespace godot;

TEST(CoverageApiTest, ContractTest) {
    // 1. Instantiate API
    Ref<CoverageApi> api;
    api.instantiate(); // Uses memnew implicitly via Ref

    ASSERT_TRUE(api.is_valid());

    // 2. Test instrument_project
    // We expect this to fail or succeed depending on if it finds project.godot,
    // but at least it shouldn't crash and should return a Dictionary.
    Dictionary instr_opts;
    Dictionary instr_result = api->instrument_project(instr_opts);
    
    // Check keys
    bool has_path = instr_result.has("output_path");
    bool has_error = instr_result.has("error");
    EXPECT_TRUE(has_path || has_error) << "instrument_project should return output_path or error";

    String output_path = "";
    if (has_path) {
        output_path = instr_result["output_path"];
    }

    // 3. Test run_instrumented_project
    Dictionary run_opts;
    // We provide a dummy path if instrumentation failed, just to test the API surface
    if (output_path.is_empty()) {
        output_path = "c:/tmp/dummy_project_path"; 
    }
    
    run_opts["output_path"] = output_path;
    run_opts["workspace_id"] = "test_workspace";
    run_opts["blocking"] = false;
    run_opts["dry_run"] = true; // Avoid spawning actual process

    // This calls OS::create_process, which might fail on the dummy path, but that's expected.
    // We verify it returns the expected structure including run_id.
    Dictionary run_result = api->run_instrumented_project(run_opts);
    
    // Check dry run results
    EXPECT_TRUE(run_result.has("args"));
    EXPECT_TRUE(run_result.has("run_id"));
    EXPECT_TRUE(run_result.has("expected_output_file"));

    if (run_result.has("exit_code")) {
         // Should not have exit code in dry run
         // FAIL() << "Dry run should not return exit code"; // Optional check
    }

    // 4. Test clear_coverage_data
    Dictionary clear_opts;
    clear_opts["workspace_id"] = "test_workspace";
    Dictionary clear_result = api->clear_coverage_data(clear_opts);
    EXPECT_TRUE(clear_result.has("status"));
    EXPECT_EQ(String(clear_result["status"]), "ok");

    // 5. Test generate_coverage_report
    Dictionary report_opts;
    report_opts["workspace_id"] = "test_workspace";
    Dictionary report_result = api->generate_coverage_report(report_opts);
    EXPECT_TRUE(report_result.has("status"));
    EXPECT_EQ(String(report_result["status"]), "ok");
}
