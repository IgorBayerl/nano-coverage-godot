#include "coverage_panel.h"

#include <algorithm>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

void CoveragePanel::_bind_methods() {
}

void CoveragePanel::setup() {
    summary_label = memnew(Label);
    summary_label->set_text("No coverage data.");
    add_child(summary_label);

    file_tree = memnew(Tree);
    file_tree->set_columns(3);
    file_tree->set_column_titles_visible(true);
    file_tree->set_column_title(0, "File");
    file_tree->set_column_title(1, "Coverage");
    file_tree->set_column_title(2, "Lines");
    file_tree->set_column_expand_ratio(0, 4);
    file_tree->set_column_expand_ratio(1, 1);
    file_tree->set_column_expand_ratio(2, 1);
    file_tree->set_hide_root(true);
    file_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(file_tree);
}

void CoveragePanel::update_data(const LcovReport& report) {
    if (!file_tree || !summary_label) return;

    file_tree->clear();
    TreeItem* root = file_tree->create_item();

    if (report.files.empty()) {
        summary_label->set_text("No coverage data.");
        return;
    }

    summary_label->set_text(
        "Total: " + String::num(report.total_coverage_percent(), 1) + "% (" +
        String::num_int64(report.total_lines_hit) + "/" +
        String::num_int64(report.total_lines_found) + " lines)"
    );

    // Sort files by coverage % ascending (worst first)
    std::vector<const FileCoverage*> sorted;
    for (const auto& kv : report.files) {
        sorted.push_back(&kv.second);
    }
    std::sort(sorted.begin(), sorted.end(),
        [](const FileCoverage* a, const FileCoverage* b) {
            return a->coverage_percent() < b->coverage_percent();
        });

    for (const FileCoverage* file : sorted) {
        TreeItem* item = file_tree->create_item(root);
        item->set_text(0, String(file->source_file.c_str()));
        item->set_text(1, String::num(file->coverage_percent(), 1) + "%");
        item->set_text(2, String::num_int64(file->lines_hit) + "/" +
                          String::num_int64(file->lines_found));

        Color color;
        float pct = file->coverage_percent();
        if (pct >= 80.0f) {
            color = Color(0.2, 0.8, 0.2);
        } else if (pct >= 50.0f) {
            color = Color(0.9, 0.7, 0.1);
        } else {
            color = Color(0.9, 0.2, 0.2);
        }
        item->set_custom_color(1, color);
        item->set_metadata(0, String(file->source_file.c_str()));
    }
}

void CoveragePanel::clear_data() {
    if (summary_label) {
        summary_label->set_text("No coverage data.");
    }
    if (file_tree) {
        file_tree->clear();
    }
}

} // namespace godot
