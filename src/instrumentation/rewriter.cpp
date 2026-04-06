#include "rewriter.h"

#include <algorithm>

namespace godot {

std::string Rewriter::apply(const std::string& src, std::vector<TextInsertion> insertions) {
    std::string out = src;

    // Sort descending by byte_offset. 
    // If offsets are equal, sort descending by original_index so the later 
    // insertion is applied first, pushing the earlier insertion to the top.
    std::sort(insertions.begin(), insertions.end(),
        [](const TextInsertion& a, const TextInsertion& b) { 
            if (a.byte_offset != b.byte_offset) {
                return a.byte_offset > b.byte_offset; 
            }
            return a.original_index > b.original_index;
        });

    for (const auto& ins : insertions) {
        if (ins.byte_offset > out.size()) {
            continue;
        }
        out.insert(ins.byte_offset, ins.text);
    }

    return out;
}

}  // namespace godot