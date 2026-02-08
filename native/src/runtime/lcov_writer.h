#pragma once
#include <godot_cpp/variant/string.hpp>

#include "../config/settings_gateway.h"
#include "../data/persistence.h"

namespace godot {

class LCOVWriter {
   public:
    static void write_lcov_report(const CoverageData& data, const CoverageSettings& settings);
};

}  // namespace godot