#pragma once

#include <cstdint>
#include <string>

namespace godot
{

    class Mapping
    {
    public:
        // Stable hash for a path like "res://foo/bar.gd"
        static uint64_t fnv1a_64(const std::string &s);
        static uint64_t hash_res_path(const std::string &res_path);
    };

} // namespace godot
