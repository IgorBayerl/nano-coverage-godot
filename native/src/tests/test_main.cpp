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
#include "test_utils.h" // Include the helper

namespace godot {

// --- Custom Listener to pipe GTest output to Godot ---
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
    
    // Use a temp folder to avoid polluting project root
    String temp_dir = "res://temp_session_test";
    TestUtils::clean_dir(temp_dir); // Ensure clean start
    DirAccess::make_dir_recursive_absolute(temp_dir);

    // Override "nano_coverage/output_dir" so save_session() writes there
    SettingsOverride s1("nano_coverage/output_dir", temp_dir);

    cov->reset();
    cov->hit(test_file, 10);
    cov->hit(test_file, 10);

    // 2. Assert Hits in Memory
    EXPECT_EQ(cov->get_total_hit_count(), 2);
    
    // 3. Test Save
    cov->save_session();
    
    // Verify file exists (optional, but good for sanity)
    EXPECT_TRUE(FileAccess::file_exists(temp_dir + "/coverage.data"));

    // 4. Cleanup
    TestUtils::clean_dir(temp_dir);
    memdelete(cov);
}

// --- RUNNER IMPLEMENTATION ---

void NanoCoverageTestRunner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("run_all_tests"), &NanoCoverageTestRunner::run_all_tests);
}

int NanoCoverageTestRunner::run_all_tests() {
    {
        Ref<FileAccess> f = FileAccess::open("res://test_log.txt", FileAccess::WRITE);
        if (f.is_valid()) f->store_line("--- STARTING TESTS ---");
    }

    UtilityFunctions::print("NanoCoverage: --- STARTING GOOGLE TEST SUITE ---");
    
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