#include "coverage_api.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <gtest/gtest.h>

using namespace godot;

TEST(CoverageApiTest, ContractTest) {
    // Instantiate API
    Ref<CoverageApi> api;
    api.instantiate();

    ASSERT_TRUE(api.is_valid());

    // Test instrument_script
    String source_code = "func foo():\n\tpass";
    String file_path = "res://foo.gd";
    Dictionary instr_result = api->instrument_script(source_code, file_path);

    // Verify success immediately
    ASSERT_TRUE(bool(instr_result.get("success", false)))
        << "Instrumentation failed: " << String(instr_result.get("error", "")).utf8().get_data();

    ASSERT_TRUE(instr_result.has("code")) << "instrument_script missing code";
    ASSERT_TRUE(instr_result.has("lines")) << "instrument_script missing lines";

    api->save_static_metadata();

    String output_path = "res://";


    // Test generate_coverage_report
    // Must run before clearing data, as it requires coverage.meta
    Dictionary report_opts;
    report_opts["workspace_id"] = "test_workspace";
    Dictionary report_result = api->generate_coverage_report(report_opts);

    ASSERT_FALSE(report_result.has("error"))
        << "Report generation failed: " << String(report_result.get("error", "")).utf8().get_data();

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