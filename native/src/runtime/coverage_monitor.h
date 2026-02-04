#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "coverage_collector.h"

namespace godot {

class NanoCoverage : public Object {
    GDCLASS(NanoCoverage, Object)

   protected:
    static void _bind_methods();

   public:
    NanoCoverage() = default;
    ~NanoCoverage() override = default;

    // Called by injected code: NanoCoverage.hit("res://foo.gd", 10)
    void hit(String file_path, int32_t line);

    void reset();

    // Dumps current memory execution data to "coverage.data" (appends).
    void save_session();

    // Reads "coverage.meta" and "coverage.data", merges them, and outputs "lcov.info".
    void generate_report();

    // Debugging helpers
    int64_t get_total_hit_count() const;
    Dictionary get_snapshot() const;  // Returns raw internal state

   private:
    CoverageCollector collector;
};
}  // namespace godot