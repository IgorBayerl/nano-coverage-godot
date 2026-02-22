#pragma once

#include <godot_cpp/variant/string.hpp>
#include <mutex>
#include <string>

#include "../data/persistence.h"

namespace godot {

class CoverageCollector {
   public:
    // Records a hit for a specific file and line.
    // Thread-safe.
    void record_hit(const String& file, int line);

    // Returns a copy of the current coverage data.
    // Thread-safe.
    CoverageData snapshot() const;

    // Clears all recorded data.
    // Thread-safe.
    void clear();

    // Returns the total number of hits recorded.
    // Thread-safe.
    uint64_t get_total_hits() const;

   private:
    mutable std::mutex mtx;
    CoverageData data;
    uint64_t total_hits = 0;
};

}  // namespace godot
