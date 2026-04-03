#include "coverage_gutter.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/text_edit.hpp>

namespace godot {

Ref<ImageTexture> CoverageGutter::s_marker_texture;

Ref<ImageTexture> CoverageGutter::get_marker_texture() {
    if (s_marker_texture.is_valid()) {
        return s_marker_texture;
    }
    // Create a small white square; gutter item_color will tint it.
    Ref<Image> img = Image::create(4, 4, false, Image::FORMAT_RGBA8);
    img->fill(Color(1, 1, 1, 1));
    s_marker_texture = ImageTexture::create_from_image(img);
    return s_marker_texture;
}

int CoverageGutter::find_gutter(CodeEdit* editor) {
    for (int i = 0; i < editor->get_gutter_count(); i++) {
        if (editor->get_gutter_name(i) == GUTTER_NAME) {
            return i;
        }
    }
    return -1;
}

void CoverageGutter::apply(CodeEdit* editor, const FileCoverage& coverage) {
    if (!editor || coverage.line_data.empty()) return;

    int gutter_idx = find_gutter(editor);
    if (gutter_idx == -1) {
        editor->add_gutter(0);
        gutter_idx = 0;
        editor->set_gutter_name(gutter_idx, GUTTER_NAME);
        editor->set_gutter_type(gutter_idx, TextEdit::GUTTER_TYPE_ICON);
        editor->set_gutter_width(gutter_idx, 6);
    }

    Ref<ImageTexture> marker = get_marker_texture();
    int line_count = editor->get_line_count();
    Color green(0.2, 0.8, 0.2, 0.8);
    Color red(0.9, 0.2, 0.2, 0.8);

    for (int line_0 = 0; line_0 < line_count; line_0++) {
        uint32_t line_1 = line_0 + 1; // LCOV is 1-based
        auto it = coverage.line_data.find(line_1);

        if (it == coverage.line_data.end()) {
            // Not a coverable line — clear any previous marker
            editor->set_line_gutter_icon(line_0, gutter_idx, Ref<Texture2D>());
            continue;
        }

        editor->set_line_gutter_item_color(
            line_0, gutter_idx,
            it->second > 0 ? green : red
        );
        editor->set_line_gutter_icon(line_0, gutter_idx, marker);
    }
}

void CoverageGutter::clear(CodeEdit* editor) {
    if (!editor) return;
    int gutter_idx = find_gutter(editor);
    if (gutter_idx != -1) {
        editor->remove_gutter(gutter_idx);
    }
}

} // namespace godot
