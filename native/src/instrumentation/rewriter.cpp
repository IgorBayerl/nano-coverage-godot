#include "rewriter.h"

#include <algorithm>

namespace godot {

std::string Rewriter::apply(const std::string& src, std::vector<TextInsertion> insertions) {
    std::string out = src;

    // We use stable_sort to guarantee deterministic output order
    // when multiple injections target the exact same byte offset.
    std::stable_sort(insertions.begin(), insertions.end(),
                     [](const TextInsertion& a, const TextInsertion& b) { return a.byte_offset > b.byte_offset; });

    for (const auto& ins : insertions) {
        if (ins.byte_offset > out.size()) {
            continue;
        }
        out.insert(ins.byte_offset, ins.text);
    }

    return out;
}

}  // namespace godot