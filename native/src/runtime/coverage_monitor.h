#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class NanoCoverage : public Object {
    GDCLASS(NanoCoverage, Object)

   protected:
    static void _bind_methods();

   public:
    NanoCoverage() = default;
    ~NanoCoverage() override = default;

    // CHANGED: Accepts String path (matches Instrumenter output)
    void hit(String file_path, int32_t line);

    void reset();

    // Trigger to write data to disk
    void save_report(String path);

    // Returns { "res://file.gd" -> { line: hits } }
    Dictionary get_snapshot() const;

    // Helper mostly for debug
    Dictionary get_line_hits(String file_path) const;

    int64_t get_total_hit_count() const;
};

}  // namespace godot