#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../runtime/lcov_writer.h"
#include "../runtime/coverage_monitor.h"

namespace fs = std::filesystem;
using namespace godot;

TEST(LCOVFormatTest, GeneratesValidReport) {
    // 1. Setup Data
    NanoCoverage* cov = memnew(NanoCoverage);
    cov->reset();

    // File A: player.gd
    // Line 10: 2 hits
    // Line 15: 1 hit
    cov->hit("res://scripts/player.gd", 10);
    cov->hit("res://scripts/player.gd", 10);
    cov->hit("res://scripts/player.gd", 15);

    // File B: enemy.gd
    // Line 5: 1 hit
    cov->hit("res://scripts/enemy.gd", 5);

    // Get snapshot
    Dictionary snapshot = cov->get_snapshot();

    // 2. Write Report
    fs::path temp_file = fs::temp_directory_path() / "lcov_test_output.info";
    // Ensure we start fresh
    if (fs::exists(temp_file)) {
        fs::remove(temp_file);
    }
    
    LCOVWriter::write_lcov_report(String(temp_file.string().c_str()), snapshot);

    // 3. Read & Verify
    std::ifstream in(temp_file);
    ASSERT_TRUE(in.is_open()) << "Failed to open generated LCOV file at " << temp_file;

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    
    // Check for File A
    // Note: The order of files in Dictionary iteration is not guaranteed to be deterministic 
    // across all platforms/Godot versions, but usually it's stable enough or we check existence.
    // However, LCOVWriter iterates the keys.
    // We should strictly check for content presence.

    EXPECT_NE(content.find("SF:res://scripts/player.gd"), std::string::npos);
    EXPECT_NE(content.find("DA:10,2"), std::string::npos);
    EXPECT_NE(content.find("DA:15,1"), std::string::npos);

    // Check for File B
    EXPECT_NE(content.find("SF:res://scripts/enemy.gd"), std::string::npos);
    EXPECT_NE(content.find("DA:5,1"), std::string::npos);

    // Check end_of_record
    // Should appear twice
    int eor_count = 0;
    size_t pos = 0;
    while ((pos = content.find("end_of_record", pos)) != std::string::npos) {
        eor_count++;
        pos += std::string("end_of_record").length();
    }
    EXPECT_EQ(eor_count, 2);

    // 4. Cleanup
    memdelete(cov);
    in.close(); // Close before deleting
    fs::remove(temp_file);
}
