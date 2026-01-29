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
    bool has_singleton = Engine::get_singleton()->has_singleton("NanoCoverage");
    EXPECT_TRUE(has_singleton) << "CRITICAL: 'NanoCoverage' singleton is NOT registered with the Engine.";
    if (has_singleton) {
        Object* obj = Engine::get_singleton()->get_singleton("NanoCoverage");
        ASSERT_NE(obj, nullptr) << "CRITICAL: Retrieved singleton is null.";
        NanoCoverage* coverage = Object::cast_to<NanoCoverage>(obj);
        EXPECT_NE(coverage, nullptr)
            << "CRITICAL: Singleton object is not of type NanoCoverage! (Possible shadowing by GDScript)";
    }
}

TEST(NanoCoverageTest, RecordsHitsAndSavesSession) {
    // 1. Setup
    NanoCoverage* cov = memnew(NanoCoverage);
    String test_file = "res://test_unit_gen.gd";

    cov->reset();
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);

    // 2. Assert Hits in Memory
    EXPECT_EQ(cov->get_total_hit_count(), 2);

    // 3. Test Save (We can't easily assert the file path in unit test without mocking,
    //    but we ensure the method call is valid and doesn't crash).
    cov->save_session();

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