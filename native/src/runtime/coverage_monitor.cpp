#include "coverage_monitor.h"

#include <mutex>
#include <unordered_map>

#include <godot_cpp/core/class_db.hpp>

namespace godot
{

    namespace
    {

        // Per-file coverage data
        struct FileCoverage
        {
            std::unordered_map<int32_t, uint32_t> line_hits;
            uint64_t total_hits = 0;
        };

        // Collector storage (process-lifetime, guarded by a mutex)
        struct Collector
        {
            mutable std::mutex mtx;
            std::unordered_map<uint64_t, FileCoverage> files;
            uint64_t total_hits = 0;

            void clear_locked()
            {
                files.clear();
                total_hits = 0;
            }
        };

        Collector &collector()
        {
            static Collector c;
            return c;
        }

    } // namespace

    void NanoCoverage::_bind_methods()
    {
        ClassDB::bind_method(D_METHOD("hit", "file_hash", "line"), &NanoCoverage::hit);
        ClassDB::bind_method(D_METHOD("reset"), &NanoCoverage::reset);
        ClassDB::bind_method(D_METHOD("get_line_hits", "file_hash"), &NanoCoverage::get_line_hits);
        ClassDB::bind_method(D_METHOD("get_snapshot"), &NanoCoverage::get_snapshot);
        ClassDB::bind_method(D_METHOD("get_total_hit_count"), &NanoCoverage::get_total_hit_count);
    }

    void NanoCoverage::hit(int64_t file_hash, int32_t line)
    {
        // Ignore invalid calls (keeps instrumentation bugs from polluting stats)
        if (file_hash == 0)
        {
            return;
        }
        if (line <= 0)
        {
            return;
        }

        Collector &c = collector();
        std::lock_guard<std::mutex> lock(c.mtx);

        FileCoverage &fc = c.files[(uint64_t)file_hash];
        fc.total_hits += 1;
        fc.line_hits[line] += 1;

        c.total_hits += 1;
    }

    void NanoCoverage::reset()
    {
        Collector &c = collector();
        std::lock_guard<std::mutex> lock(c.mtx);
        c.clear_locked();
    }

    Dictionary NanoCoverage::get_line_hits(int64_t file_hash) const
    {
        Dictionary out;

        Collector &c = collector();
        std::lock_guard<std::mutex> lock(c.mtx);

        auto it = c.files.find((uint64_t)file_hash);
        if (it == c.files.end())
        {
            return out;
        }

        const FileCoverage &fc = it->second;
        for (const auto &kv : fc.line_hits)
        {
            // kv.first: line (int32), kv.second: hits (uint32)
            out[kv.first] = (int64_t)kv.second;
        }

        return out;
    }

    Dictionary NanoCoverage::get_snapshot() const
    {
        Dictionary out;

        Collector &c = collector();
        std::lock_guard<std::mutex> lock(c.mtx);

        for (const auto &file_kv : c.files)
        {
            const uint64_t file_hash = file_kv.first;
            const FileCoverage &fc = file_kv.second;

            Dictionary lines;
            for (const auto &line_kv : fc.line_hits)
            {
                lines[line_kv.first] = (int64_t)line_kv.second;
            }

            out[(int64_t)file_hash] = lines;
        }

        return out;
    }

    int64_t NanoCoverage::get_total_hit_count() const
    {
        Collector &c = collector();
        std::lock_guard<std::mutex> lock(c.mtx);
        return (int64_t)c.total_hits;
    }

} // namespace godot
