#include "test_main.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <gtest/gtest.h>
#include <sstream>

#include "../runtime/coverage_monitor.h"
#include "test_utils.h"

namespace godot {

// CUSTOM TEST LISTENER DOCUMENTATION
//
// Why do we need this class?
// Standard C++ std::cout/std::cerr output is often captured or suppressed by the Godot Engine,
// especially in Editor or Embedded environments. To ensure test results are visible:
//
// We must use `UtilityFunctions::print()`/`printerr()` to route messages through Godot's
// internal logging system (which displays in the Output dock and console).
//
// We optionally log to a file ("res://test_log.txt") to ensure results are preserved even
// if the engine crashes or the console buffer is cleared.
//
// This listener hooks into Google Test's event system to intercept test results and
// print/log them using Godot's API.
class GodotGTestPrinter : public ::testing::EmptyTestEventListener {
    void log_to_file(String message) {
        Ref<FileAccess> f;
        if (FileAccess::file_exists("res://test_log.txt")) {
            f = FileAccess::open("res://test_log.txt", FileAccess::READ_WRITE);
            f->seek_end();
        } else {
            f = FileAccess::open("res://test_log.txt", FileAccess::WRITE);
        }
        if (f.is_valid()) {
            f->store_line(message);
        }
    }

    virtual void OnTestPartResult(const ::testing::TestPartResult& result) {
        if (result.failed()) {
            std::stringstream ss;
            ss << "\n[FAILED] " << result.file_name() << ":" << result.line_number() << "\n"
               << "         " << result.summary() << "\n";
            String msg = String(ss.str().c_str());
            UtilityFunctions::printerr(msg);
            log_to_file(msg);
        }
    }

    virtual void OnTestEnd(const ::testing::TestInfo& test_info) {
        if (test_info.result()->Failed()) {
            String msg = "[TEST FAILED] " + String(test_info.test_suite_name()) + "." + String(test_info.name());
            UtilityFunctions::printerr(msg);
            log_to_file(msg);
        } else {
            String msg = "[TEST PASSED] " + String(test_info.test_suite_name()) + "." + String(test_info.name());
            UtilityFunctions::print(msg);
            log_to_file(msg);
        }
    }
};

// TEST CASES

// Checks if the NanoCoverage singleton is properly registered with the Godot Engine.
// It verifies that we can retrieve it by name and that it is the correct C++ type.
TEST(EngineIntegrationTest, SingletonIsRegisteredAndAccessible) {
    bool has_singleton = Engine::get_singleton()->has_singleton("NanoCoverage");
    ASSERT_TRUE(has_singleton) << "CRITICAL: 'NanoCoverage' singleton is NOT registered with the Engine.";

    Object* obj = Engine::get_singleton()->get_singleton("NanoCoverage");
    ASSERT_NE(obj, nullptr) << "CRITICAL: Retrieved singleton is null.";
    NanoCoverage* coverage = Object::cast_to<NanoCoverage>(obj);
    EXPECT_NE(coverage, nullptr) << "CRITICAL: Singleton object is not of type NanoCoverage! (Possible shadowing by GDScript)";
}

// Verifies that we can record coverage hits in memory and that save_session()
// writes the coverage data to a file on disk.
TEST(NanoCoverageTest, RecordsHitsAndSavesSession) {
    // Setup
    NanoCoverage* cov = memnew(NanoCoverage);
    String test_file = "res://test_unit_gen.gd";
    
    // Use a temp folder to avoid polluting project root
    String temp_dir = "res://temp_session_test";
    TestUtils::clean_dir(temp_dir); // Ensure clean start
    DirAccess::make_dir_recursive_absolute(temp_dir);

    // Override "nano_coverage/output_dir" so save_session() writes there
    SettingsOverride s1("nano_coverage/output_dir", temp_dir);

    cov->reset();
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);

    // Assert Hits in Memory
    EXPECT_EQ(cov->get_total_hit_count(), 2);
    
    // Test Save
    cov->save_session();
    
    // Verify file exists (optional, but good for sanity)
    EXPECT_TRUE(FileAccess::file_exists(temp_dir + "/coverage.data"));

    // Cleanup
    TestUtils::clean_dir(temp_dir);
    memdelete(cov);
}

// RUNNER IMPLEMENTATION

void NanoCoverageTestRunner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("run_all_tests"), &NanoCoverageTestRunner::run_all_tests);
}

// How Tests are Run:
// ------------------
// The invocation chain starts with scripts/test.py (Python).
// The script creates a temporary GDScript file (addons/run_cpp_tests.gd) which
// instantiates NanoCoverageTestRunner (this class) and calls run_all_tests().
//
// Python launches Godot in --headless mode, executing the GDScript.
// NanoCoverageTestRunner initializes Google Test within the running Godot process.
// It runs ALL registered Google Tests (TEST() macros) found in the binary.
// Results are printed back to Godot, which Python captures via stdout.
int NanoCoverageTestRunner::run_all_tests() {
    {
        Ref<FileAccess> f = FileAccess::open("res://test_log.txt", FileAccess::WRITE);
        if (f.is_valid()) f->store_line("--- STARTING TESTS ---");
    }

    UtilityFunctions::print("NanoCoverage: --- STARTING GTEST SUITE ---");
    
    PackedStringArray user_args = OS::get_singleton()->get_cmdline_user_args();
    int argc = user_args.size() + 1;
    std::vector<char*> argv_vec;
    std::vector<std::string> arg_strings;

    arg_strings.push_back("nano_coverage_tests");
    for (const String& arg : user_args) {
        arg_strings.push_back(arg.utf8().get_data());
    }

    for (size_t i = 0; i < arg_strings.size(); ++i) {
        argv_vec.push_back((char*)arg_strings[i].c_str());
    }
    argv_vec.push_back(nullptr);
    
    char** argv = argv_vec.data();

    if (!::testing::GTEST_FLAG(list_tests)) {
        ::testing::InitGoogleTest(&argc, argv);
        ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
        listeners.Append(new GodotGTestPrinter);
    }

    int res = RUN_ALL_TESTS();
    if (res == 0) {
        UtilityFunctions::print("NanoCoverage: --- ALL TESTS PASSED ---");
    } else {
        UtilityFunctions::printerr("NanoCoverage: --- TESTS FAILED ---");
    }

    return res;
}

}  // namespace godot