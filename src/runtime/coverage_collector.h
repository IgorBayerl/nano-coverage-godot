#pragma once

#include <godot_cpp/variant/string.hpp>
#include <atomic>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "../data/persistence.h"

namespace godot {

struct AtomicCounter {
    std::atomic<uint64_t> count{0};
    AtomicCounter() = default;
    AtomicCounter(const AtomicCounter& other) {
        count.store(other.count.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
    AtomicCounter& operator=(const AtomicCounter& other) {
        count.store(other.count.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }
};

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
    mutable std::shared_mutex mtx;
    std::unordered_map<std::string, std::unordered_map<uint32_t, AtomicCounter>> atomic_data;
    std::atomic<uint64_t> total_hits{0};
};

}  // namespace godot
