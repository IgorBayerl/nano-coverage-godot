#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

class Logger {
   public:
    static void info(const String& msg);
    static void warn(const String& msg);
    static void error(const String& msg);
};

}  // namespace godot
