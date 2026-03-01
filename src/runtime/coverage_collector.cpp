#include "coverage_collector.h"

namespace godot {

void CoverageCollector::record_hit(const String& file, int line) {
    if (file.is_empty() || line <= 0) {
        return;
    }

    std::string path_std = file.utf8().get_data();

    {
        // Try fast path with read lock
        std::shared_lock<std::shared_mutex> read_lock(mtx);
        auto it_file = atomic_data.find(path_std);
        if (it_file != atomic_data.end()) {
            auto it_line = it_file->second.find(line);
            if (it_line != it_file->second.end()) {
                it_line->second.count.fetch_add(1, std::memory_order_relaxed);
                total_hits.fetch_add(1, std::memory_order_relaxed);
                return;
            }
        }
    }

    // Fallback: need to insert, requires write lock
    std::lock_guard<std::shared_mutex> write_lock(mtx);
    atomic_data[path_std][line].count.fetch_add(1, std::memory_order_relaxed);
    total_hits.fetch_add(1, std::memory_order_relaxed);
}

CoverageData CoverageCollector::snapshot() const {
    CoverageData result;
    std::shared_lock<std::shared_mutex> lock(mtx);
    for (const auto& [file, lines] : atomic_data) {
        for (const auto& [line, counter] : lines) {
            result[file][line] = counter.count.load(std::memory_order_relaxed);
        }
    }
    return result;
}

void CoverageCollector::clear() {
    std::lock_guard<std::shared_mutex> lock(mtx);
    atomic_data.clear();
    total_hits.store(0, std::memory_order_relaxed);
}

uint64_t CoverageCollector::get_total_hits() const {
    return total_hits.load(std::memory_order_relaxed);
}

}  // namespace godot
