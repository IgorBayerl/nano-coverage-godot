#include "nano_coverage/test_runner.hpp"

// Include Google Test
#include <gtest/gtest.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <sstream>

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
            // UtilityFunctions::print("[TEST PASSED] ", test_info.test_suite_name(), ".", test_info.name());
        }
    }
};

void NanoCoverageTestRunner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("run_all_tests"), &NanoCoverageTestRunner::run_all_tests);
}

int NanoCoverageTestRunner::run_all_tests() {
    UtilityFunctions::print("NanoCoverage: --- STARTING GOOGLE TEST SUITE ---");

    int argc = 1;
    char* argv[] = { (char*)"nano_coverage_tests", nullptr };
    
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

} // namespace godot