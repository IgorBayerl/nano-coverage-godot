#include "lcov_parser.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace fs = std::filesystem;
using namespace godot;

TEST(LcovParserTest, ParsesBasicReport) {
    std::string lcov =
        "TN:\n"
        "SF:src/player.gd\n"
        "DA:3,5\n"
        "DA:4,0\n"
        "DA:5,2\n"
        "LF:3\n"
        "LH:2\n"
        "end_of_record\n";

    LcovReport report = LcovParser::parse_string(lcov);

    ASSERT_EQ(report.files.size(), 1u);
    auto& file = report.files["src/player.gd"];
    EXPECT_EQ(file.lines_found, 3u);
    EXPECT_EQ(file.lines_hit, 2u);
    EXPECT_EQ(file.line_data[3], 5u);
    EXPECT_EQ(file.line_data[4], 0u);
    EXPECT_EQ(file.line_data[5], 2u);
    EXPECT_NEAR(file.coverage_percent(), 66.666f, 0.01f);
    EXPECT_NEAR(report.total_coverage_percent(), 66.666f, 0.01f);
}

TEST(LcovParserTest, ParsesMultipleFiles) {
    std::string lcov =
        "TN:\nSF:a.gd\nDA:1,1\nLF:1\nLH:1\nend_of_record\n"
        "TN:\nSF:b.gd\nDA:1,0\nLF:1\nLH:0\nend_of_record\n";

    LcovReport report = LcovParser::parse_string(lcov);

    ASSERT_EQ(report.files.size(), 2u);
    EXPECT_EQ(report.total_lines_found, 2u);
    EXPECT_EQ(report.total_lines_hit, 1u);
    EXPECT_NEAR(report.total_coverage_percent(), 50.0f, 0.01f);
}

TEST(LcovParserTest, HandlesEmptyInput) {
    LcovReport report = LcovParser::parse_string("");
    EXPECT_EQ(report.files.size(), 0u);
    EXPECT_FLOAT_EQ(report.total_coverage_percent(), 0.0f);
}

TEST(LcovParserTest, HandlesMalformedDALines) {
    std::string lcov =
        "SF:test.gd\n"
        "DA:bad\n"
        "DA:3,1\n"
        "LF:1\n"
        "LH:1\n"
        "end_of_record\n";

    LcovReport report = LcovParser::parse_string(lcov);
    EXPECT_EQ(report.files["test.gd"].line_data.size(), 1u);
    EXPECT_EQ(report.files["test.gd"].line_data[3], 1u);
}

TEST(LcovParserTest, HandlesWindowsLineEndings) {
    std::string lcov =
        "TN:\r\n"
        "SF:win.gd\r\n"
        "DA:1,3\r\n"
        "LF:1\r\n"
        "LH:1\r\n"
        "end_of_record\r\n";

    LcovReport report = LcovParser::parse_string(lcov);
    ASSERT_EQ(report.files.size(), 1u);
    EXPECT_EQ(report.files["win.gd"].line_data[1], 3u);
}

TEST(LcovParserTest, ParsesFileFromDisk) {
    fs::path temp = fs::temp_directory_path() / "lcov_parser_test.info";

    {
        std::ofstream f(temp);
        f << "TN:\nSF:disk.gd\nDA:1,7\nLF:1\nLH:1\nend_of_record\n";
    }

    LcovReport report = LcovParser::parse_file(temp.string());
    ASSERT_EQ(report.files.size(), 1u);
    EXPECT_EQ(report.files["disk.gd"].line_data[1], 7u);

    fs::remove(temp);
}

TEST(LcovParserTest, ParseFileReturnsEmptyOnMissingFile) {
    LcovReport report = LcovParser::parse_file("/nonexistent/path/lcov.info");
    EXPECT_EQ(report.files.size(), 0u);
}

TEST(LcovParserTest, IgnoresUnknownPrefixes) {
    std::string lcov =
        "TN:test_name\n"
        "SF:x.gd\n"
        "FN:1,func_name\n"
        "FNDA:3,func_name\n"
        "FNF:1\n"
        "FNH:1\n"
        "DA:1,3\n"
        "LF:1\n"
        "LH:1\n"
        "end_of_record\n";

    LcovReport report = LcovParser::parse_string(lcov);
    ASSERT_EQ(report.files.size(), 1u);
    EXPECT_EQ(report.files["x.gd"].lines_found, 1u);
}
