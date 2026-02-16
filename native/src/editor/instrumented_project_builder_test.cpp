#include "instrumented_project_builder.h"

#include <filesystem>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>  // Required for the new signature
#include <gtest/gtest.h>
#include <string>

namespace fs = std::filesystem;
using namespace godot;

TEST(InstrumentedProjectBuilderTest, CreatesValidProjectStructure) {
    // 1. Action
    // We must pass a Dictionary now (even if empty)
    Dictionary options;
    String result_path_str = InstrumentedProjectBuilder::build_instrumented_project(options);

    // Convert to standard string/path
    std::string path_std = result_path_str.utf8().get_data();
    fs::path result_path(path_std);

    // 2. Assertions

    // Check path string is not empty
    EXPECT_FALSE(result_path_str.is_empty()) << "Temp path returned empty string";

    // Check directory existence
    ASSERT_TRUE(fs::exists(result_path)) << "Temp directory does not exist on disk";
    EXPECT_TRUE(fs::is_directory(result_path));

    // Check Project file (Mandatory Copy)
    EXPECT_TRUE(fs::exists(result_path / "project.godot")) << "project.godot missing";

    // Check Main Scene (Verifies general file traversal)
    EXPECT_TRUE(fs::exists(result_path / "main.tscn")) << "main.tscn missing";

    // Check Plugin Script (Verifies recursive copy + .gd filtering)
    fs::path plugin_script = result_path / "addons" / "nano_coverage_godot" / "plugin.gd";
    EXPECT_TRUE(fs::exists(plugin_script)) << "addons/nano_coverage_godot/plugin.gd missing";

    // Check Caches (Verifies .godot folder handling)
    EXPECT_TRUE(fs::exists(result_path / ".godot" / "uid_cache.bin")) << "UID cache missing";

    // Verify the Class Cache is copied (Fixes the GdUnit4 Parse Errors)
    EXPECT_TRUE(fs::exists(result_path / ".godot" / "global_script_class_cache.cfg"))
        << "global_script_class_cache.cfg missing (Critical for plugins like GdUnit4)";
}