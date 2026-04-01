#pragma once

#include <godot_cpp/variant/string.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace godot {

struct FileCoverage {
    std::string source_file;
    uint32_t lines_found = 0;
    uint32_t lines_hit = 0;
    // line_number (1-based) -> hit_count
    std::unordered_map<uint32_t, uint64_t> line_data;

    float coverage_percent() const {
        return lines_found > 0 ? (float)lines_hit / (float)lines_found * 100.0f : 0.0f;
    }
};

struct LcovReport {
    std::unordered_map<std::string, FileCoverage> files;
    uint32_t total_lines_found = 0;
    uint32_t total_lines_hit = 0;

    float total_coverage_percent() const {
        return total_lines_found > 0
            ? (float)total_lines_hit / (float)total_lines_found * 100.0f
            : 0.0f;
    }
};

class LcovParser {
public:
    /// Parse LCOV content from a string.
    static LcovReport parse_string(const std::string& content);

    /// Parse an LCOV file from disk. Returns empty report on failure.
    static LcovReport parse_file(const std::string& path);
};

} // namespace godot
