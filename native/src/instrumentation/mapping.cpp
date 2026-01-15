#include "mapping.h"

namespace godot
{

    uint64_t Mapping::fnv1a_64(const std::string &s)
    {
        uint64_t h = 14695981039346656037ull;
        for (unsigned char c : s)
        {
            h ^= (uint64_t)c;
            h *= 1099511628211ull;
        }
        return h;
    }

    uint64_t Mapping::hash_res_path(const std::string &res_path)
    {
        return fnv1a_64(res_path);
    }

} // namespace godot
