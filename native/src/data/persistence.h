#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace godot {

// Map: FilePath -> List of Line Numbers
using CoverageMetadata = std::unordered_map<std::string, std::vector<uint32_t>>;

// Map: FilePath -> (Line Number -> Hit Count)
using CoverageData = std::unordered_map<std::string, std::unordered_map<uint32_t, uint64_t>>;

class Persistence {
   public:
    // Writes the static metadata (files and their coverable lines) to a binary file.
    // Overwrites existing file.
    static bool save_metadata(const std::string& path, const CoverageMetadata& metadata);

    // Reads metadata from disk.
    static bool load_metadata(const std::string& path, CoverageMetadata& out_metadata);

    // Appends the current execution hits to a binary file.
    // Supports appending multiple sessions to the same file.
    static bool append_execution_data(const std::string& path, const CoverageData& data);

    // Reads an execution data file, merging all sessions found within it.
    static bool load_and_merge_execution_data(const std::string& path, CoverageData& out_data);
};

}  // namespace godot