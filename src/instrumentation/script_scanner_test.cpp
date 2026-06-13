#include "script_scanner.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <gtest/gtest.h>

#include "../testing/test_utils.h"

using namespace godot;

static const char* SCAN_TEST_DIR = "res://_test_script_scanner";

class ScriptScannerTest : public ::testing::Test {
   protected:
    String TEST_DIR;

    void SetUp() override {
        TEST_DIR = SCAN_TEST_DIR;
        TestUtils::clean_dir(TEST_DIR);
        DirAccess::make_dir_recursive_absolute(TEST_DIR);
    }

    void TearDown() override {
        TestUtils::clean_dir(TEST_DIR);
    }

    static void write_file(const String& path, const String& content) {
        String dir = path.get_base_dir();
        if (!DirAccess::dir_exists_absolute(dir)) {
            DirAccess::make_dir_recursive_absolute(dir);
        }
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
        f->store_string(content);
        f->close();
    }
};

TEST_F(ScriptScannerTest, FindsOnlyGdFilesRecursively) {
    write_file(TEST_DIR.path_join("a.gd"), "extends Node\n");
    write_file(TEST_DIR.path_join("nested/deep/b.gd"), "extends Node\n");
    write_file(TEST_DIR.path_join("readme.txt"), "not a script\n");
    write_file(TEST_DIR.path_join("scene.tscn"), "[gd_scene]\n");

    Array files = ScriptScanner::find_gd_files(TEST_DIR, Array());

    ASSERT_EQ(files.size(), 2);
    EXPECT_TRUE(files.has(TEST_DIR.path_join("a.gd")));
    EXPECT_TRUE(files.has(TEST_DIR.path_join("nested/deep/b.gd")));
}

TEST_F(ScriptScannerTest, GlobStarMatchesAcrossDirectories) {
    write_file(TEST_DIR.path_join("src/main.gd"), "extends Node\n");
    write_file(TEST_DIR.path_join("src/sub/main_test.gd"), "extends Node\n");
    write_file(TEST_DIR.path_join("other_test.gd"), "extends Node\n");

    Array globs;
    globs.push_back("**/*_test.gd");
    Array compiled = ScriptScanner::compile_ignore_patterns(globs);

    Array files = ScriptScanner::find_gd_files(TEST_DIR, compiled);

    ASSERT_EQ(files.size(), 1);
    EXPECT_TRUE(files.has(TEST_DIR.path_join("src/main.gd")));
}

TEST_F(ScriptScannerTest, SingleStarDoesNotCrossDirectories) {
    write_file(TEST_DIR.path_join("skip_me.gd"), "extends Node\n");
    write_file(TEST_DIR.path_join("nested/skip_me.gd"), "extends Node\n");

    Array globs;
    // Matches only at the test dir root, not in nested directories.
    globs.push_back(String(SCAN_TEST_DIR) + "/skip*.gd");
    Array compiled = ScriptScanner::compile_ignore_patterns(globs);

    Array files = ScriptScanner::find_gd_files(TEST_DIR, compiled);

    ASSERT_EQ(files.size(), 1);
    EXPECT_TRUE(files.has(TEST_DIR.path_join("nested/skip_me.gd")));
}

TEST_F(ScriptScannerTest, IgnoredDirectoriesAreNotEntered) {
    write_file(TEST_DIR.path_join("keep/a.gd"), "extends Node\n");
    write_file(TEST_DIR.path_join("ignored/b.gd"), "extends Node\n");

    Array globs;
    globs.push_back(String(SCAN_TEST_DIR) + "/ignored**");
    Array compiled = ScriptScanner::compile_ignore_patterns(globs);

    Array files = ScriptScanner::find_gd_files(TEST_DIR, compiled);

    ASSERT_EQ(files.size(), 1);
    EXPECT_TRUE(files.has(TEST_DIR.path_join("keep/a.gd")));
}

TEST_F(ScriptScannerTest, BuildIgnoreGlobsRespectsAddonSetting) {
    CoverageSettings settings;
    settings.ignore_paths.push_back("**/*_test.gd");

    settings.ignore_addons = true;
    Array globs_all_addons = ScriptScanner::build_ignore_globs(settings);
    EXPECT_TRUE(globs_all_addons.has("res://addons/**"));

    settings.ignore_addons = false;
    Array globs_own_addon = ScriptScanner::build_ignore_globs(settings);
    EXPECT_FALSE(globs_own_addon.has("res://addons/**"));
    // Our own addon is always excluded
    EXPECT_TRUE(globs_own_addon.has("res://addons/nano_coverage_godot/**"));
}
