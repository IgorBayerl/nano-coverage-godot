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

    // Debugging helpers
    int64_t get_total_hit_count() const;

   private:
    CoverageCollector collector;
};
}  // namespace godot