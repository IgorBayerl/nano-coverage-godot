#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace godot {

struct TextInsertion {
    size_t byte_offset = 0;
    std::string text;
    size_t original_index = 0;
};

class Rewriter {
   public:
    static std::string apply(const std::string& src, std::vector<TextInsertion> insertions);
};

}  // namespace godot