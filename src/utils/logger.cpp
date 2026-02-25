#include "logger.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void Logger::info(const String& msg) {
    UtilityFunctions::print(String("[NanoCoverage] ") + msg);
}

void Logger::warn(const String& msg) {
    UtilityFunctions::print(String("[NanoCoverage] [WARN] ") + msg);
}

void Logger::error(const String& msg) {
    UtilityFunctions::printerr(String("[NanoCoverage] [ERROR] ") + msg);
}

}  // namespace godot
