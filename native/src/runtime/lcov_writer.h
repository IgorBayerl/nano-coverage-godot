#pragma once
#include "../config/settings_gateway.h"
#include "../data/persistence.h"
#include <godot_cpp/variant/string.hpp>

namespace godot {

class LCOVWriter {
   public:
    static void write_lcov_report(const CoverageData& data, const CoverageSettings& settings);
};

}  // namespace godot