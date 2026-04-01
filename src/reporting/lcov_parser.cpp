#include "lcov_parser.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace godot {

// Safe string-to-unsigned-long conversion without exceptions.
// Returns false if the string is not a valid number.
static bool safe_stoul(const std::string& str, uint32_t& out) {
    if (str.empty()) return false;
    char* end = nullptr;
    unsigned long val = std::strtoul(str.c_str(), &end, 10);
    if (end == str.c_str() || *end != '\0') return false;
    out = static_cast<uint32_t>(val);
    return true;
}

static bool safe_stoull(const std::string& str, uint64_t& out) {
    if (str.empty()) return false;
    char* end = nullptr;
    unsigned long long val = std::strtoull(str.c_str(), &end, 10);
    if (end == str.c_str() || *end != '\0') return false;
    out = static_cast<uint64_t>(val);
    return true;
}

LcovReport LcovParser::parse_string(const std::string& content) {
    LcovReport report;
    FileCoverage* current = nullptr;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Strip trailing \r for Windows line endings
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.rfind("SF:", 0) == 0) {
            std::string path = line.substr(3);
            current = &report.files[path];
            current->source_file = path;
        } else if (line.rfind("DA:", 0) == 0 && current) {
            auto comma = line.find(',', 3);
            if (comma != std::string::npos) {
                uint32_t ln = 0;
                uint64_t hits = 0;
                if (safe_stoul(line.substr(3, comma - 3), ln) &&
                    safe_stoull(line.substr(comma + 1), hits)) {
                    current->line_data[ln] = hits;
                }
            }
        } else if (line.rfind("LF:", 0) == 0 && current) {
            uint32_t val = 0;
            if (safe_stoul(line.substr(3), val)) {
                current->lines_found = val;
                report.total_lines_found += val;
            }
        } else if (line.rfind("LH:", 0) == 0 && current) {
            uint32_t val = 0;
            if (safe_stoul(line.substr(3), val)) {
                current->lines_hit = val;
                report.total_lines_hit += val;
            }
        } else if (line == "end_of_record") {
            current = nullptr;
        }
    }
    return report;
}

LcovReport LcovParser::parse_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return parse_string(content);
}

} // namespace godot
