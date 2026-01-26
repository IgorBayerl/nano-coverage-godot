#include "coverage_monitor.h"

#include <godot_cpp/core/class_db.hpp>
#include <mutex>
#include <string>
#include <unordered_map>

#include "lcov_writer.h"  // Ensure this exists

namespace godot {

namespace {

// Per-file coverage data
struct FileCoverage {
    std::unordered_map<int32_t, uint32_t> line_hits;
    uint64_t total_hits = 0;
};

// Collector storage
struct Collector {
    mutable std::mutex mtx;

    // We store data by Hash (fast lookups)
    std::unordered_map<int64_t, FileCoverage> files;

    // We map Hash -> Real Path (for the final report)
    std::unordered_map<int64_t, String> id_to_path;

    uint64_t total_hits = 0;

    void clear_locked() {
        files.clear();
        id_to_path.clear();
        total_hits = 0;
    }
};

Collector& collector() {
    static Collector c;
    return c;
}

}  // namespace

void NanoCoverage::_bind_methods() {
    // CHANGED: "file_path" instead of "file_hash"
    ClassDB::bind_method(D_METHOD("hit", "file_path", "line"), &NanoCoverage::hit);

    // NEW: Register save_report so GDScript can call it
    ClassDB::bind_method(D_METHOD("save_report", "path"), &NanoCoverage::save_report);

    ClassDB::bind_method(D_METHOD("reset"), &NanoCoverage::reset);
    ClassDB::bind_method(D_METHOD("get_line_hits", "file_path"), &NanoCoverage::get_line_hits);
    ClassDB::bind_method(D_METHOD("get_snapshot"), &NanoCoverage::get_snapshot);
    ClassDB::bind_method(D_METHOD("get_total_hit_count"), &NanoCoverage::get_total_hit_count);
}

// 1. Fix Logic: Accept String, Hash it internally, cache the path
void NanoCoverage::hit(String file_path, int32_t line) {
    if (file_path.is_empty() || line <= 0) {
        return;
    }

    // Use Godot's built-in string hash
    int64_t h = file_path.hash();

    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);

    // If this is the first time we see this file, remember the path string
    if (c.id_to_path.find(h) == c.id_to_path.end()) {
        c.id_to_path[h] = file_path;
    }

    FileCoverage& fc = c.files[h];
    fc.total_hits += 1;
    fc.line_hits[line] += 1;

    c.total_hits += 1;
}

// 2. Fix Scope: This function is now properly inside the namespace
void NanoCoverage::save_report(String path) {
    // We get a snapshot with real paths as keys
    Dictionary snap = get_snapshot();

    // Pass to your writer (which you still need to implement!)
    LCOVWriter::write_lcov_report(path, snap);
}

void NanoCoverage::reset() {
    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);
    c.clear_locked();
}

Dictionary NanoCoverage::get_line_hits(String file_path) const {
    Dictionary out;
    int64_t h = file_path.hash();

    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);

    auto it = c.files.find(h);
    if (it == c.files.end()) {
        return out;
    }

    const FileCoverage& fc = it->second;
    for (const auto& kv : fc.line_hits) {
        out[kv.first] = (int64_t)kv.second;
    }
    return out;
}

Dictionary NanoCoverage::get_snapshot() const {
    Dictionary out;

    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);

    for (const auto& file_kv : c.files) {
        const int64_t file_hash = file_kv.first;
        const FileCoverage& fc = file_kv.second;

        // RESOLVE HASH TO PATH
        // We use the string path as the key in the Dictionary so the
        // LCOVWriter doesn't have to worry about hashes.
        String path_str = "unknown";
        auto path_it = c.id_to_path.find(file_hash);
        if (path_it != c.id_to_path.end()) {
            path_str = path_it->second;
        }

        Dictionary lines;
        for (const auto& line_kv : fc.line_hits) {
            lines[line_kv.first] = (int64_t)line_kv.second;
        }

        out[path_str] = lines;
    }

    return out;
}

int64_t NanoCoverage::get_total_hit_count() const {
    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);
    return (int64_t)c.total_hits;
}

}  // namespace godot