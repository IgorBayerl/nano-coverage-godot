#include "lcov_writer.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "../config/settings_gateway.h"
#include "../data/persistence.h"

namespace fs = std::filesystem;
using namespace godot;

TEST(LCOVFormatTest, GeneratesValidReport) {
    // Setup Data
    CoverageData data;

    // File A: player.gd
    // Line 10: 2 hits
    // Line 15: 1 hit
    data["res://scripts/player.gd"][10] = 2;
    data["res://scripts/player.gd"][15] = 1;

    // File B: enemy.gd
    // Line 5: 1 hit
    data["res://scripts/enemy.gd"][5] = 1;

    // Setup Settings
    fs::path temp_dir = fs::temp_directory_path();
    CoverageSettings settings;
    settings.report_dir = String(temp_dir.string().c_str());
    settings.report_lcov_filename = "lcov_test_output.info";

    // Clean up previous run
    fs::path output_file = temp_dir / "lcov_test_output.info";
    if (fs::exists(output_file)) {
        fs::remove(output_file);
    }

    // Write Report
    LCOVWriter::write_lcov_report(data, settings);

    // Read & Verify
    std::ifstream in(output_file);
    ASSERT_TRUE(in.is_open()) << "Failed to open generated LCOV file at " << output_file;

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    // Check for File A
    EXPECT_NE(content.find("SF:scripts/player.gd"), std::string::npos);
    EXPECT_NE(content.find("DA:10,2"), std::string::npos);
    EXPECT_NE(content.find("DA:15,1"), std::string::npos);

    // Check for File B
    EXPECT_NE(content.find("SF:scripts/enemy.gd"), std::string::npos);
    EXPECT_NE(content.find("DA:5,1"), std::string::npos);

    // Check end_of_record
    int eor_count = 0;
    size_t pos = 0;
    while ((pos = content.find("end_of_record", pos)) != std::string::npos) {
        eor_count++;
        pos += std::string("end_of_record").length();
    }
    EXPECT_EQ(eor_count, 2);

    // Cleanup
    in.close();
    fs::remove(output_file);
}



