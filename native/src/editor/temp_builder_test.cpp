#include <gtest/gtest.h>

#include "temp_builder.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace godot;

TEST(TempProjectBuilderTest, CreatesValidProjectStructure) {
    
    // 1. Action
    String result_path_str = TempProjectBuilder::create_temp_project();
    
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
    // We use this because we know it exists in the addons folder
    fs::path plugin_script = result_path / "addons" / "nano_coverage_godot" / "plugin.gd";
    EXPECT_TRUE(fs::exists(plugin_script)) << "addons/nano_coverage_godot/plugin.gd missing";
    
    // Check Caches (Verifies .godot folder handling)
    EXPECT_TRUE(fs::exists(result_path / ".godot" / "uid_cache.bin")) << "UID cache missing";
}