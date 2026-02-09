#include "persistence.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <unordered_map>

namespace fs = std::filesystem;
using namespace godot;

TEST(PersistenceTest, MetadataSaveAndLoad) {
    std::string path = "test_meta.bin";

    CoverageMetadata in_meta;
    in_meta["res://a.gd"] = {1, 2, 5};
    in_meta["res://b.gd"] = {10, 20};

    // Save
    ASSERT_TRUE(Persistence::save_metadata(path, in_meta));
    ASSERT_TRUE(fs::exists(path));

    // Load
    CoverageMetadata out_meta;
    ASSERT_TRUE(Persistence::load_metadata(path, out_meta));

    // Verify
    EXPECT_EQ(out_meta.size(), 2);
    EXPECT_EQ(out_meta["res://a.gd"].size(), 3);
    EXPECT_EQ(out_meta["res://b.gd"].size(), 2);
    EXPECT_EQ(out_meta["res://a.gd"][2], 5);

    fs::remove(path);
}

TEST(PersistenceTest, ExecutionDataAppendAndMerge) {
    std::string path = "test_data.bin";
    if (fs::exists(path))
        fs::remove(path);

    // Session 1
    CoverageData s1;
    s1["res://game.gd"][5] = 10;  // 10 hits on line 5
    ASSERT_TRUE(Persistence::append_execution_data(path, s1));

    // Session 2
    CoverageData s2;
    s2["res://game.gd"][5] = 5;  // 5 more hits on line 5
    s2["res://game.gd"][6] = 1;  // 1 hit on line 6
    ASSERT_TRUE(Persistence::append_execution_data(path, s2));

    // Load & Merge
    CoverageData result;
    ASSERT_TRUE(Persistence::load_and_merge_execution_data(path, result));

    // Verify
    EXPECT_EQ(result["res://game.gd"][5], 15);  // 10 + 5
    EXPECT_EQ(result["res://game.gd"][6], 1);   // 0 + 1

    fs::remove(path);
}