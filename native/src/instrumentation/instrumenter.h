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

    // I/O Wrapper: reads file, instruments, writes back.
    static bool instrument_file(const String& path, const String& res_path, std::vector<uint32_t>* out_lines = nullptr,
                                int* out_insertions = nullptr);

    // Deprecated: kept for compatibility if needed internally, but likely replaced by instrument_file.
    // We can remove it if we update all callers. Attempting to remove it to stay clean.
};

}  // namespace godot