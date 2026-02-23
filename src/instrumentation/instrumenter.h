#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "godot_cpp/variant/string.hpp"

namespace godot {

struct InstrumentResult {
    bool ok = false;
    std::string instrumented_code;
    std::vector<uint32_t> covered_lines;
    int insertions = 0;
    std::string error_message;
};

class Instrumenter {
   public:
    // Pure logic: takes code, returns instrumented code. No I/O.
    static InstrumentResult instrument_text(const std::string& utf8_code, const std::string& res_path);
};

}  // namespace godot