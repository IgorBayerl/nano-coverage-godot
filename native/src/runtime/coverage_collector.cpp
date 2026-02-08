#include "coverage_collector.h"

namespace godot {

void CoverageCollector::record_hit(const String& file, int line) {
    if (file.is_empty() || line <= 0) {
        return;
    }

    std::string path_std = file.utf8().get_data();

    std::lock_guard<std::mutex> lock(mtx);
    data[path_std][line]++;
    total_hits++;
}

CoverageData CoverageCollector::snapshot() const {
    std::lock_guard<std::mutex> lock(mtx);
    return data;
}

void CoverageCollector::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    data.clear();
    total_hits = 0;
}

uint64_t CoverageCollector::get_total_hits() const {
    std::lock_guard<std::mutex> lock(mtx);
    return total_hits;
}

}  // namespace godot
