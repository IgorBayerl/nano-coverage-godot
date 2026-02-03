#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../instrumentation/instrumenter.h"

namespace fs = std::filesystem;
using namespace godot;

// Helper to normalize line endings to \n
static std::string normalize_newlines(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\r') {
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                continue; // Skip \r followed by \n
            }
        }
        output.push_back(input[i]);
    }
    return output;
}

TEST(InstrumentationGoldenTest, BaselineBehavior) {
    // 1. Setup Input
    const std::string gd_source = R"(extends Node

func _ready():
	var x = 10
	if x > 5:
		print("Hello")

# A comment
)";

    // 2. Define Expected Output
    // Current instrumentation logic:
    // - Line 4 (var x = 10) is instrumented.
    // - Line 5 (if x > 5) is instrumented.
    // - Line 6 (print) is instrumented.
    const std::string expected_output = R"(extends Node

func _ready():
	NanoCoverage.hit("res://test_golden.gd", 4)
	var x = 10
	NanoCoverage.hit("res://test_golden.gd", 5)
	if x > 5:
		NanoCoverage.hit("res://test_golden.gd", 6)
		print("Hello")

# A comment
)";

    // 3. Create Temp File
    fs::path temp_file = fs::temp_directory_path() / "golden_test_input.gd";
    {
        std::ofstream out(temp_file, std::ios::binary);
        out << gd_source;
    }

    // 4. Run Instrumenter
    std::vector<uint32_t> covered_lines;
    int insertions = 0;
    // We pass a dummy "res://" path to ensure deterministic output in the hit() call
    bool success = Instrumenter::instrument_file_in_place(
        temp_file, 
        "res://test_golden.gd", 
        covered_lines, 
        &insertions
    );

    ASSERT_TRUE(success) << "Instrumentation failed";

    // 5. Read Result
    std::string actual_output;
    {
        std::ifstream in(temp_file, std::ios::binary);
        ASSERT_TRUE(in.is_open());
        // Read file into string
        in.seekg(0, std::ios::end);
        actual_output.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&actual_output[0], actual_output.size());
    }

    // 6. Cleanup
    fs::remove(temp_file);

    // 7. Verify
    // Normalize both to ensure test passes on Windows/Linux regardless of checkout settings
    std::string norm_expected = normalize_newlines(expected_output);
    std::string norm_actual = normalize_newlines(actual_output);

    EXPECT_EQ(norm_actual, norm_expected);
    
    // Also verify strict line reporting
    // Lines 4, 5, 6 should be in covered_lines
    ASSERT_EQ(covered_lines.size(), 3);
    EXPECT_EQ(covered_lines[0], 4);
    EXPECT_EQ(covered_lines[1], 5);
    EXPECT_EQ(covered_lines[2], 6);
}
