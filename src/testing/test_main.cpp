#include "test_main.h"

#include "test_utils.h"

// GDExtension Includes
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Google Test
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>

// Project Includes
// ADJUST THIS PATH: Point to where 'NanoCoverage' class is defined
#include "../api/coverage_api.h"
// OR
#include "../runtime/coverage_monitor.h"

using namespace godot;

// --- Custom Test Listener ---
// Captures GTest output and redirects it to Godot's console and a log file.
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

   public:  // Must be public for GTest to call
    virtual void OnTestPartResult(const ::testing::TestPartResult& result) override {
        if (result.failed()) {
            std::stringstream ss;
            ss << "\n[FAILED] " << result.file_name() << ":" << result.line_number() << "\n"
               << "         " << result.summary() << "\n";
            String msg = String(ss.str().c_str());
            UtilityFunctions::printerr(msg);
            log_to_file(msg);
        }
    }

    virtual void OnTestEnd(const ::testing::TestInfo& test_info) override {
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

// --- Test Cases ---

TEST(EngineIntegrationTest, SingletonIsRegisteredAndAccessible) {
    bool has_singleton = Engine::get_singleton()->has_singleton("NanoCoverage");
    ASSERT_TRUE(has_singleton) << "CRITICAL: 'NanoCoverage' singleton is NOT registered with the Engine.";

    Object* obj = Engine::get_singleton()->get_singleton("NanoCoverage");
    ASSERT_NE(obj, nullptr) << "CRITICAL: Retrieved singleton is null.";

    NanoCoverage* coverage = Object::cast_to<NanoCoverage>(obj);
    EXPECT_NE(coverage, nullptr)
        << "CRITICAL: Singleton object is not of type NanoCoverage! (Possible shadowing by GDScript)";
}

TEST(NanoCoverageTest, RecordsHitsAndSavesSession) {
    // Setup
    // Use memnew for Godot objects to ensure proper memory tracking
    NanoCoverage* cov = memnew(NanoCoverage);
    String test_file = "res://test_unit_gen.gd";

    // Use a temp folder to avoid polluting project root
    String temp_dir = "res://temp_session_test";

    // Clean before test
    TestUtils::clean_dir(temp_dir);
    DirAccess::make_dir_recursive_absolute(temp_dir);

    // Override "nano_coverage/output_dir" so save_session() writes there
    SettingsOverride s1("nano_coverage/output_dir", temp_dir);

    // Act
    cov->reset();
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);

    // Assert Hits in Memory
    EXPECT_EQ(cov->get_total_hit_count(), 2);

    // Test Save
    cov->save_session();

    // Verify file exists
    // Note: coverage.data is the default name, check your constant in NanoCoverage
    EXPECT_TRUE(FileAccess::file_exists(temp_dir + "/coverage.data"));

    // Cleanup
    TestUtils::clean_dir(temp_dir);
    memdelete(cov);
}

// --- Runner Implementation ---

void NanoCoverageTestRunner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("run_all_tests"), &NanoCoverageTestRunner::run_all_tests);
}

int NanoCoverageTestRunner::run_all_tests() {
    // Clear log file
    {
        Ref<FileAccess> f = FileAccess::open("res://test_log.txt", FileAccess::WRITE);
        if (f.is_valid())
            f->store_line("--- STARTING TESTS ---");
    }

    UtilityFunctions::print("NanoCoverage: --- STARTING GTEST SUITE ---");

    // Convert Godot Args to C-style argv for GTest
    PackedStringArray user_args = OS::get_singleton()->get_cmdline_user_args();
    int argc = user_args.size() + 1;
    std::vector<char*> argv_vec;
    std::vector<std::string> arg_strings;

    // First arg is program name
    arg_strings.push_back("nano_coverage_tests");

    // Add user args
    for (const String& arg : user_args) {
        arg_strings.push_back(arg.utf8().get_data());
    }

    // Convert std::string to char*
    for (size_t i = 0; i < arg_strings.size(); ++i) {
        argv_vec.push_back((char*)arg_strings[i].c_str());
    }
    argv_vec.push_back(nullptr);  // Null terminator

    char** argv = argv_vec.data();

    // Initialize GTest (once per process usually, but Godot reloads libs)
    // We check a flag to avoid re-init if possible, or just init every run.
    ::testing::InitGoogleTest(&argc, argv);

    // Remove default listener (stdout) and add ours
    // (Optional: Keep default if you want stdout logs too)
    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new GodotGTestPrinter);

    // Run Tests
    int res = RUN_ALL_TESTS();

    if (res == 0) {
        UtilityFunctions::print("NanoCoverage: --- ALL TESTS PASSED ---");
    } else {
        UtilityFunctions::printerr("NanoCoverage: --- TESTS FAILED ---");
    }

    return res;
}