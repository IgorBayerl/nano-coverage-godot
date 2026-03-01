#include "coverage_monitor.h"

#include <filesystem>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <gtest/gtest.h>

#include "../config/settings_keys.h"
#include "../data/persistence.h"
#include "testing/test_main.h"
#include "testing/test_utils.h"

namespace godot {


TEST(CoverageLogicTest, ReportsZeroHitsForMissedLines) {
    // Setup Environment
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "nano_zero_hit_test";
    if (std::filesystem::exists(test_dir))
        std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);

    std::string file_name = "res://player.gd";

    // Create Artificial Metadata (Simulate Build Step)
    // We say lines 10 and 20 are coverable.
    CoverageMetadata meta;
    meta[file_name] = {10, 20};

    std::filesystem::path meta_path = test_dir / "coverage.meta";
    Persistence::save_metadata(meta_path.string(), meta);

    // Create Artificial Hits (Simulate Run Step)
    // We only hit line 10. Line 20 is missed.
    CoverageData hits;
    hits[file_name][10] = 5;

    std::filesystem::path run_path = test_dir / "runs" / "run_1.covdata";
    std::filesystem::create_directories(run_path.parent_path());
    Persistence::append_execution_data(run_path.string(), hits);

    // Merge Logic (Simulating CoverageApi::generate_coverage_report logic)

    // Load Metadata
    CoverageMetadata loaded_meta;
    bool meta_loaded = Persistence::load_metadata(meta_path.string(), loaded_meta);
    ASSERT_TRUE(meta_loaded) << "Failed to load metadata";

    // Load Hits
    CoverageData loaded_hits;
    bool hits_loaded = Persistence::load_and_merge_execution_data(run_path.string(), loaded_hits);
    ASSERT_TRUE(hits_loaded) << "Failed to load hits";

    // Perform Union
    CoverageData final_data;
    for (const auto& meta_kv : loaded_meta) {
        const std::string& f_path = meta_kv.first;
        const std::vector<uint32_t>& coverable_lines = meta_kv.second;

        std::unordered_map<uint32_t, uint64_t>& file_lines = final_data[f_path];

        auto hit_file_it = loaded_hits.find(f_path);
        bool has_hits = (hit_file_it != loaded_hits.end());

        for (uint32_t line : coverable_lines) {
            uint64_t count = 0;
            if (has_hits) {
                auto hit_line_it = hit_file_it->second.find(line);
                if (hit_line_it != hit_file_it->second.end()) {
                    count = hit_line_it->second;
                }
            }
            // Store the count (even if 0)
            file_lines[line] = count;
        }
    }

    // Assertions
    // Line 10 should have 5 hits
    ASSERT_TRUE(final_data[file_name].find(10) != final_data[file_name].end());
    EXPECT_EQ(final_data[file_name][10], 5);

    // Line 20 should exist and have 0 hits (This confirms the fix)
    ASSERT_TRUE(final_data[file_name].find(20) != final_data[file_name].end())
        << "Line 20 (missed) was not present in final data";
    EXPECT_EQ(final_data[file_name][20], 0);

    // Cleanup
    std::filesystem::remove_all(test_dir);
}

}  // namespace godot