#include <gtest/gtest.h>
#include "../utils/logger.h"

using namespace godot;

TEST(LoggerTest, FormatExecutionNoCrash) {
    Logger::info("Test Info");
    Logger::warn("Test Warning");
    Logger::error("Test Error");
    SUCCEED();
}
