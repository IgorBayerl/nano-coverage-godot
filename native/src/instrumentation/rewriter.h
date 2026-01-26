#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace godot {

struct TextInsertion {
    size_t byte_offset = 0;
    std::string text;
};

class Rewriter {
   public:
    // Applies insertions in descending byte_offset order.
    static std::string apply(const std::string& src, std::vector<TextInsertion> insertions);
};

}  // namespace godot
