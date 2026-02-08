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
	else:
		print("Else")

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
	else:
		NanoCoverage.hit("res://test_golden.gd", 8)
		print("Else")

# A comment
)";


    // 4. Run Instrumenter
    // We pass a dummy "res://" path to ensure deterministic output in the hit() call
    InstrumentResult result = Instrumenter::instrument_text(gd_source, "res://test_golden.gd");

    ASSERT_TRUE(result.ok) << "Instrumentation failed: " << result.error_message;

    // 5. Cleanup
    // (No file I/O to cleanup)

    // 6. Verify
    // Normalize both to ensure test passes on Windows/Linux regardless of checkout settings
    std::string norm_expected = normalize_newlines(expected_output);
    std::string norm_actual = normalize_newlines(result.instrumented_code);

    EXPECT_EQ(norm_actual, norm_expected);
    
    // Also verify strict line reporting
    // Lines 4, 5, 6 should be in covered_lines
    // Lines 4, 5, 6, 8 should be in covered_lines
    ASSERT_EQ(result.covered_lines.size(), 4);
    EXPECT_EQ(result.covered_lines[0], 4);
    EXPECT_EQ(result.covered_lines[1], 5);
    EXPECT_EQ(result.covered_lines[2], 6);
    EXPECT_EQ(result.covered_lines[3], 8);
}
