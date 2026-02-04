#include <gtest/gtest.h>
#include "../runtime/coverage_store.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace godot;

class CoverageStoreTest : public ::testing::Test {
protected:
    fs::path temp_dir;
    std::string root_dir;
    std::string workspace_id;

    void SetUp() override {
        // Create unique temp dir for each test
        temp_dir = fs::temp_directory_path() / ("cov_store_test_" + std::to_string(std::rand()));
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directories(temp_dir);
        
        root_dir = temp_dir.string();
        workspace_id = "test_ws";
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }
};

TEST_F(CoverageStoreTest, AppendAndLoadSingleRun) {
    CoverageStore store(root_dir, workspace_id);
    
    CoverageData run1;
    run1["res://file1.gd"][1] = 5;
    run1["res://file1.gd"][2] = 2;
    
    store.append_run_snapshot("run_1", run1);
    
    CoverageData loaded = store.load_and_merge();
    
    ASSERT_EQ(loaded.size(), 1);
    ASSERT_EQ(loaded["res://file1.gd"].size(), 2);
    EXPECT_EQ(loaded["res://file1.gd"][1], 5);
    EXPECT_EQ(loaded["res://file1.gd"][2], 2);
}

TEST_F(CoverageStoreTest, MergeMultipleRuns) {
    CoverageStore store(root_dir, workspace_id);
    
    CoverageData run1;
    run1["res://file1.gd"][1] = 5;
    run1["res://file1.gd"][2] = 10;
    
    CoverageData run2;
    run2["res://file1.gd"][1] = 3;  // Should sum to 8
    run2["res://file2.gd"][5] = 1;  // New file
    
    store.append_run_snapshot("run_A", run1);
    store.append_run_snapshot("run_B", run2);
    
    CoverageData loaded = store.load_and_merge();
    
    ASSERT_EQ(loaded.size(), 2); // file1.gd, file2.gd
    
    // Check file1.gd
    EXPECT_EQ(loaded["res://file1.gd"][1], 8); // 5 + 3
    EXPECT_EQ(loaded["res://file1.gd"][2], 10); // 10 + 0
    
    // Check file2.gd
    EXPECT_EQ(loaded["res://file2.gd"][5], 1);
}

TEST_F(CoverageStoreTest, HandlesEmptyStore) {
    CoverageStore store(root_dir, workspace_id);
    
    CoverageData loaded = store.load_and_merge();
    EXPECT_TRUE(loaded.empty());
}
