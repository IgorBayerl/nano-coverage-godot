#pragma once

#include <cstdint>

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{

    /// GDScript-facing singleton:
    ///   NanoCoverage.hit(file_hash, line)
    ///   NanoCoverage.get_line_hits(file_hash)
    ///   NanoCoverage.get_snapshot()
    ///   NanoCoverage.reset()
    class NanoCoverage : public Object
    {
        GDCLASS(NanoCoverage, Object)

    protected:
        static void _bind_methods();

    public:
        NanoCoverage() = default;
        ~NanoCoverage() override = default;

        /// Record one hit for (file_hash, line).
        void hit(int64_t file_hash, int32_t line);

        /// Clears all collected data.
        void reset();

        /// Returns { line:int -> hits:int } for a single file hash.
        Dictionary get_line_hits(int64_t file_hash) const;

        /// Returns { file_hash:int -> { line:int -> hits:int } }
        Dictionary get_snapshot() const;

        /// Total number of hit() calls recorded across all files/lines.
        int64_t get_total_hit_count() const;
    };

} // namespace godot
