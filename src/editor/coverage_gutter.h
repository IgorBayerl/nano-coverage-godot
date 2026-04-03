#pragma once

#include <godot_cpp/classes/code_edit.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include "../reporting/lcov_parser.h"

namespace godot {

class CoverageGutter {
public:
    // Apply gutter markers to the CodeEdit based on coverage data.
    static void apply(CodeEdit* editor, const FileCoverage& coverage);

    // Remove our gutter from CodeEdit.
    static void clear(CodeEdit* editor);

private:
    static constexpr const char* GUTTER_NAME = "nano_coverage";

    // Find our gutter index, or -1 if not present.
    static int find_gutter(CodeEdit* editor);

    // Lazily-created 4x4 white marker texture (tinted by item_color).
    static Ref<ImageTexture> get_marker_texture();
    static Ref<ImageTexture> s_marker_texture;
};

} // namespace godot
