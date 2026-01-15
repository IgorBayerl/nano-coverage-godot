#include "rewriter.h"

#include <algorithm>

namespace godot
{

    std::string Rewriter::apply(const std::string &src, std::vector<TextInsertion> insertions)
    {
        std::string out = src;

        std::sort(insertions.begin(), insertions.end(),
                  [](const TextInsertion &a, const TextInsertion &b)
                  { return a.byte_offset > b.byte_offset; });

        for (const auto &ins : insertions)
        {
            if (ins.byte_offset > out.size())
            {
                continue;
            }
            out.insert(ins.byte_offset, ins.text);
        }

        return out;
    }

} // namespace godot
