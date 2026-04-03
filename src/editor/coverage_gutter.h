#pragma once

#include <godot_cpp/classes/code_edit.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include "../reporting/lcov_parser.h"

namespace godot {

// RefCounted so we can bind the custom draw callback as a Callable.
// One instance is created per CodeEdit that has coverage applied.
class CoverageGutter : public RefCounted {
    GDCLASS(CoverageGutter, RefCounted)

public:
    void setup(CodeEdit* editor, const FileCoverage& coverage);
    void remove();

    // Custom draw callback — called by CodeEdit for each visible line.
    void _draw_gutter(int32_t line, int32_t gutter, const Rect2& area);

    static void clear(CodeEdit* editor);

protected:
    static void _bind_methods();

private:
    static constexpr const char* COLOR_GUTTER_NAME = "nano_cov_color";
    static constexpr const char* HITS_GUTTER_NAME = "nano_cov_hits";

    static int find_gutter(CodeEdit* editor, const char* name);

    CodeEdit* editor_ref = nullptr;
};

} // namespace godot
