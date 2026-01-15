#pragma once

#include <filesystem>
#include <string>

namespace godot
{

    class Instrumenter
    {
    public:
        // Instruments the file at abs_path in-place.
        // res_path is used for hashing, e.g. "res://folder/file.gd".
        // out_insertions (optional) returns how many hit calls were inserted.
        static bool instrument_file_in_place(const std::filesystem::path &abs_path,
                                             const std::string &res_path,
                                             int *out_insertions = nullptr);
    };

} // namespace godot
