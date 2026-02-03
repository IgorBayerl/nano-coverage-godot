#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace godot {

class Instrumenter {
   public:
    // Instruments the file at abs_path in-place.
    // res_path: used for logging/hashing (e.g., "res://scripts/player.gd")
    // out_lines: (Output) Returns a list of all line numbers identified as coverable.
    // out_insertions_count: (Output, Optional) Returns number of code injections made.
    static bool instrument_file_in_place(const std::filesystem::path& abs_path, const std::string& res_path,
                                         std::vector<uint32_t>& out_lines, int* out_insertions_count = nullptr);
};

}  // namespace godot