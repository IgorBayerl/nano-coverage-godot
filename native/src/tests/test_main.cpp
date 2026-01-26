#include "test_main.h"

// Include Google Test
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <gtest/gtest.h>
#include <sstream>

// Include the class we are testing
#include "../runtime/coverage_monitor.h"

namespace godot {

// --- Custom Listener to pipe GTest output to Godot ---
class GodotGTestPrinter : public ::testing::EmptyTestEventListener {
    // Called when a test assertion fails
    virtual void OnTestPartResult(const ::testing::TestPartResult& result) {
        if (result.failed()) {
            std::stringstream ss;
            // Format: [FAILED] filename:line summary
            ss << "\n[FAILED] " << result.file_name() << ":" << result.line_number() << "\n"
               << "         " << result.summary() << "\n";
            UtilityFunctions::printerr(ss.str().c_str());
        }
    }

    // Called after a test ends
    virtual void OnTestEnd(const ::testing::TestInfo& test_info) {
        if (test_info.result()->Failed()) {
            UtilityFunctions::printerr("[TEST FAILED] ", test_info.test_suite_name(), ".", test_info.name());
        } else {
            // Uncomment this if you want to see passing tests too
            UtilityFunctions::print("[TEST PASSED] ", test_info.test_suite_name(), ".", test_info.name());
        }
    }
};

// --- TEST CASES ---

TEST(EngineIntegrationTest, SingletonIsRegisteredAndAccessible) {
    // 1. Check if Engine has the singleton registered
    // If this fails, register_types.cpp might not be running or initialized correctly.
    bool has_singleton = Engine::get_singleton()->has_singleton("NanoCoverage");
    EXPECT_TRUE(has_singleton) << "CRITICAL: 'NanoCoverage' singleton is NOT registered with the Engine.";

    if (has_singleton) {
        // 2. Try to retrieve it
        Object* obj = Engine::get_singleton()->get_singleton("NanoCoverage");
        ASSERT_NE(obj, nullptr) << "CRITICAL: Retrieved singleton is null.";

        // 3. Verify Type safety (Crucial for the shadowing bug)
        // If a GDScript autoload shadowed it, the object found might not be our C++ class.
        NanoCoverage* coverage = Object::cast_to<NanoCoverage>(obj);
        EXPECT_NE(coverage, nullptr)
            << "CRITICAL: Singleton object is not of type NanoCoverage! (Possible shadowing by GDScript)";
    }
}

TEST(NanoCoverageTest, SavesReportToDisk) {
    // 1. Setup: Create a local instance for testing logic (isolated from global singleton)
    NanoCoverage* cov = memnew(NanoCoverage);

    // We simulate a file path and some hits
    // Note: The hit() function now caches the path string internally
    String test_file = "res://test_unit_gen.gd";

    // Hit line 10 five times
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);

    // 2. Action: Save to a temporary test file
    String report_path = "res://test_output.lcov";

    // Ensure it doesn't exist before we start
    if (FileAccess::file_exists(report_path)) {
        DirAccess::remove_absolute(report_path);
    }

    cov->save_report(report_path);

    // 3. Assertion: Check File Existence
    EXPECT_TRUE(FileAccess::file_exists(report_path)) << "LCOV report file was not created.";

    // 4. Assertion: Check File Content
    if (FileAccess::file_exists(report_path)) {
        Ref<FileAccess> f = FileAccess::open(report_path, FileAccess::READ);
        ASSERT_FALSE(f.is_null()) << "Could not open generated report.";

        String content = f->get_as_text();

        // Verify Standard LCOV tags
        EXPECT_TRUE(content.contains("SF:res://test_unit_gen.gd")) << "Missing Source File (SF) header";
        EXPECT_TRUE(content.contains("DA:10,5")) << "Missing Data Record (DA) or incorrect hit count";
        EXPECT_TRUE(content.contains("end_of_record")) << "Missing end_of_record marker";

        f->close();
    }

    // 5. Cleanup
    if (FileAccess::file_exists(report_path)) {
        DirAccess::remove_absolute(report_path);
    }
    memdelete(cov);
}

// --- RUNNER IMPLEMENTATION ---

void NanoCoverageTestRunner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("run_all_tests"), &NanoCoverageTestRunner::run_all_tests);
}

int NanoCoverageTestRunner::run_all_tests() {
    UtilityFunctions::print("NanoCoverage: --- STARTING GOOGLE TEST SUITE ---");

    int argc = 1;
    char* argv[] = {(char*)"nano_coverage_tests", nullptr};

    // Check if already initialized to avoid duplicate listener issues on repeated runs (if any)
    if (!::testing::GTEST_FLAG(list_tests)) {
        ::testing::InitGoogleTest(&argc, argv);

        // Add our custom printer to the listener list
        ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
        listeners.Append(new GodotGTestPrinter);
    }

    // Run tests
    int res = RUN_ALL_TESTS();

    if (res == 0) {
        UtilityFunctions::print("NanoCoverage: --- ALL TESTS PASSED ---");
    } else {
        UtilityFunctions::printerr("NanoCoverage: --- TESTS FAILED ---");
    }

    return res;
}

}  // namespace godot