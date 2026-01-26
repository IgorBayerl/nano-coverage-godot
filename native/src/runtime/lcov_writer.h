#pragma once
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class LCOVWriter {
   public:
    static void write_lcov_report(const String& output_path, const Dictionary& snapshot);
};

}  // namespace godot