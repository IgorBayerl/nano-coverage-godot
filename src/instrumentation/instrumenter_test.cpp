#include "instrumenter.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace godot;

// Helper to normalize line endings to \n
static std::string normalize_newlines(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\r') {
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                continue;
            }
        }
        output.push_back(input[i]);
    }
    return output;
}

TEST(InstrumentationGoldenTest, BaselineBehavior) {
    const std::string gd_source = R"(extends Node

func _ready():
	var x = 10
	if x > 5:
		print("Hello")
	else:
		print("Else")

# A comment
)";

    const std::string expected_output = R"(extends Node

func _ready():
	NanoCoverage.hit("res://test_golden.gd", 4)
	var x = 10
	NanoCoverage.hit("res://test_golden.gd", 5)
	if x > 5:
		NanoCoverage.hit("res://test_golden.gd", 6)
		print("Hello")
	else:
		NanoCoverage.hit("res://test_golden.gd", 8)
		print("Else")

# A comment
)";

    InstrumentResult result = Instrumenter::instrument_text(gd_source, "res://test_golden.gd");
    ASSERT_TRUE(result.ok) << "Instrumentation failed: " << result.error_message;

    std::string norm_expected = normalize_newlines(expected_output);
    std::string norm_actual = normalize_newlines(result.instrumented_code);

    EXPECT_EQ(norm_actual, norm_expected);

    ASSERT_EQ(result.covered_lines.size(), 4);
    EXPECT_EQ(result.covered_lines[0], 4);
    EXPECT_EQ(result.covered_lines[1], 5);
    EXPECT_EQ(result.covered_lines[2], 6);
    EXPECT_EQ(result.covered_lines[3], 8);
}

// NEW: Test our Match Statement tree-sitter logic
TEST(InstrumentationGoldenTest, MatchStatementBehavior) {
    // 1. Setup Input
    const std::string gd_source = R"(extends Node

func check_value(val):
	match val:
		1:
			print("One")
		2:
			pass
		_:
			print("Other")
)";

    // 2. Define Expected Output (Updated to the correct syntax!)
    const std::string expected_output = R"(extends Node

func check_value(val):
	NanoCoverage.hit("res://test_match.gd", 4)
	match val:
		1:
			NanoCoverage.hit("res://test_match.gd", 6)
			print("One")
		2:
			NanoCoverage.hit("res://test_match.gd", 8)
			pass
		_:
			NanoCoverage.hit("res://test_match.gd", 10)
			print("Other")
)";

    // 3. Run Instrumenter
    InstrumentResult result = Instrumenter::instrument_text(gd_source, "res://test_match.gd");
    ASSERT_TRUE(result.ok) << "Instrumentation failed: " << result.error_message;

    // 4. Verify Code Generation
    std::string norm_expected = normalize_newlines(expected_output);
    std::string norm_actual = normalize_newlines(result.instrumented_code);

    EXPECT_EQ(norm_actual, norm_expected);

    // 5. Verify Covered Lines (Lines 4, 6, 8, 10)
    ASSERT_EQ(result.covered_lines.size(), 4);
    EXPECT_EQ(result.covered_lines[0], 4);
    EXPECT_EQ(result.covered_lines[1], 6);
    EXPECT_EQ(result.covered_lines[2], 8);
    EXPECT_EQ(result.covered_lines[3], 10);
}