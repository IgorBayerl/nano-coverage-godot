#include "coverage_gutter.h"

#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Metadata values stored per-line on the color gutter.
// These move with the line when the user edits the document.
static const int META_NONE = 0;
static const int META_COVERED = 1;
static const int META_UNCOVERED = 2;

void CoverageGutter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_draw_gutter", "line", "gutter", "area"),
                         &CoverageGutter::_draw_gutter);
}

int CoverageGutter::find_gutter(CodeEdit* editor, const char* name) {
    for (int i = 0; i < editor->get_gutter_count(); i++) {
        if (editor->get_gutter_name(i) == name) {
            return i;
        }
    }
    return -1;
}

void CoverageGutter::setup(CodeEdit* editor, const FileCoverage& coverage) {
    if (!editor) return;

    editor_ref = editor;

    // --- Color bar gutter (custom draw, full line height) ---
    int color_idx = find_gutter(editor, COLOR_GUTTER_NAME);
    if (color_idx == -1) {
        editor->add_gutter(0);
        color_idx = 0;
        editor->set_gutter_name(color_idx, COLOR_GUTTER_NAME);
        editor->set_gutter_type(color_idx, TextEdit::GUTTER_TYPE_CUSTOM);
        editor->set_gutter_width(color_idx, 8);
    }
    editor->set_gutter_custom_draw(color_idx, Callable(this, "_draw_gutter"));

    // --- Hit count gutter (text) ---
    int hits_idx = find_gutter(editor, HITS_GUTTER_NAME);
    if (hits_idx == -1) {
        int insert_at = color_idx + 1;
        editor->add_gutter(insert_at);
        hits_idx = insert_at;
        editor->set_gutter_name(hits_idx, HITS_GUTTER_NAME);
        editor->set_gutter_type(hits_idx, TextEdit::GUTTER_TYPE_STRING);
        editor->set_gutter_width(hits_idx, 40);
    }

    int line_count = editor->get_line_count();
    Color hit_color(0.5, 0.8, 0.5, 0.7);
    Color miss_color(0.8, 0.4, 0.4, 0.7);

    for (int line_0 = 0; line_0 < line_count; line_0++) {
        uint32_t line_1 = line_0 + 1;
        auto it = coverage.line_data.find(line_1);

        if (it == coverage.line_data.end()) {
            // Not coverable — no marker
            editor->set_line_gutter_metadata(line_0, color_idx, META_NONE);
            editor->set_line_gutter_text(line_0, hits_idx, "");
            continue;
        }

        if (it->second > 0) {
            editor->set_line_gutter_metadata(line_0, color_idx, META_COVERED);
            editor->set_line_gutter_text(line_0, hits_idx, String::num_int64(it->second) + "x");
            editor->set_line_gutter_item_color(line_0, hits_idx, hit_color);
        } else {
            editor->set_line_gutter_metadata(line_0, color_idx, META_UNCOVERED);
            editor->set_line_gutter_text(line_0, hits_idx, "0x");
            editor->set_line_gutter_item_color(line_0, hits_idx, miss_color);
        }
    }

    editor->queue_redraw();
}

void CoverageGutter::_draw_gutter(int32_t line, int32_t gutter, const Rect2& area) {
    if (!editor_ref) return;

    // Read the per-line metadata (moves with the line on edits)
    int meta = editor_ref->get_line_gutter_metadata(line, gutter);
    if (meta == META_NONE) return;

    Color color = (meta == META_COVERED)
        ? Color(0.2, 0.8, 0.2, 0.8)   // green
        : Color(0.9, 0.2, 0.2, 0.8);  // red

    editor_ref->draw_rect(area, color);
}

void CoverageGutter::remove() {
    if (!editor_ref) return;
    clear(editor_ref);
    editor_ref = nullptr;
}

void CoverageGutter::clear(CodeEdit* editor) {
    if (!editor) return;
    int idx = find_gutter(editor, COLOR_GUTTER_NAME);
    if (idx != -1) {
        editor->set_gutter_custom_draw(idx, Callable());
        editor->remove_gutter(idx);
    }
    idx = find_gutter(editor, HITS_GUTTER_NAME);
    if (idx != -1) {
        editor->remove_gutter(idx);
    }
}

} // namespace godot
