#include "coverage_panel.h"

#include <algorithm>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "../config/settings_gateway.h"
#include "../utils/path_utils.h"

namespace godot {

void CoveragePanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_item_activated"), &CoveragePanel::_on_item_activated);
    ClassDB::bind_method(D_METHOD("_on_button_clicked"), &CoveragePanel::_on_button_clicked);
    ClassDB::bind_method(D_METHOD("_on_filter_changed", "text"), &CoveragePanel::_on_filter_changed);
    ClassDB::bind_method(D_METHOD("_on_column_title_clicked", "column", "mouse_button"), &CoveragePanel::_on_column_title_clicked);
}

void CoveragePanel::setup() {
    // Top row for summary and filter
    HBoxContainer* top_row = memnew(HBoxContainer);
    top_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(top_row);

    summary_label = memnew(Label);
    summary_label->set_text("No coverage data.");
    summary_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    top_row->add_child(summary_label);

    Label* filter_label = memnew(Label);
    filter_label->set_text("Filter:");
    top_row->add_child(filter_label);

    filter_edit = memnew(LineEdit);
    filter_edit->set_placeholder("Search (e.g. *player*, src/**/*.gd)");
    filter_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    filter_edit->set_stretch_ratio(0.5);
    filter_edit->set_clear_button_enabled(true);
    filter_edit->connect("text_changed", Callable(this, "_on_filter_changed"));
    top_row->add_child(filter_edit);

    file_tree = memnew(Tree);
    file_tree->set_columns(4);
    file_tree->set_column_titles_visible(true);
    // Column titles are dynamically set in _populate_tree to show sorting arrows
    file_tree->set_column_expand_ratio(0, 5);
    file_tree->set_column_expand_ratio(1, 1);
    file_tree->set_column_expand_ratio(2, 1);
    file_tree->set_column_expand(3, false);
    file_tree->set_column_custom_minimum_width(3, 40);
    file_tree->set_hide_root(true);
    file_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(file_tree);

    file_tree->connect("item_activated", Callable(this, "_on_item_activated"));
    file_tree->connect("button_clicked", Callable(this, "_on_button_clicked"));
    file_tree->connect("column_title_clicked", Callable(this, "_on_column_title_clicked"));
}

void CoveragePanel::_open_file(TreeItem* item) {
    if (!item) return;
    String lcov_path = item->get_metadata(0);
    if (lcov_path.is_empty()) return;

    String res_path = PathUtils::lcov_to_res(lcov_path);
    Ref<Script> script = ResourceLoader::get_singleton()->load(res_path);
    if (script.is_valid()) {
        EditorInterface::get_singleton()->edit_script(script);
    }
}

void CoveragePanel::_on_item_activated() {
    TreeItem* selected = file_tree->get_selected();
    _open_file(selected);
}

void CoveragePanel::_on_button_clicked(TreeItem* item, int32_t column, int32_t id, int32_t mouse_button) {
    if (id == 0) { // open file button
        _open_file(item);
    }
}

void CoveragePanel::_on_filter_changed(const String& text) {
    current_filter = text;
    _populate_tree();
}

void CoveragePanel::_on_column_title_clicked(int32_t column, int32_t mouse_button) {
    if (mouse_button != 1) return; // MOUSE_BUTTON_LEFT = 1 in Godot 4

    if (column == 3) return; // Action column, not sortable

    if (current_sort_column == column) {
        current_sort_ascending = !current_sort_ascending;
    } else {
        current_sort_column = column;
        // When switching to File Name, default to A-Z (Ascending). Otherwise default to Highest First (Descending)
        current_sort_ascending = (column == 0) ? true : false; 
    }
    _populate_tree();
}

void CoveragePanel::update_data(const LcovReport& report) {
    current_report = report;
    
    if (summary_label) {
        if (current_report.files.empty()) {
            summary_label->set_text("No coverage data.");
        } else {
            summary_label->set_text(
                "Total: " + String::num(current_report.total_coverage_percent(), 1) + "% (" +
                String::num_int64(current_report.total_lines_hit) + "/" +
                String::num_int64(current_report.total_lines_found) + " lines)"
            );
        }
    }

    _populate_tree();
}

void CoveragePanel::_populate_tree() {
    if (!file_tree) return;

    file_tree->clear();

    // Use Godot's String::chr() to safely render Unicode arrows regardless of C++ compiler text encoding
    String asc_arrow = " " + String::chr(0x25B2);  // Up Arrow
    String desc_arrow = " " + String::chr(0x25BC); // Down Arrow

    // Update column titles with sorting indicators
    file_tree->set_column_title(0, current_sort_column == 0 ? "File" + (current_sort_ascending ? asc_arrow : desc_arrow) : "File");
    file_tree->set_column_title(1, current_sort_column == 1 ? "Coverage" + (current_sort_ascending ? asc_arrow : desc_arrow) : "Coverage");
    file_tree->set_column_title(2, current_sort_column == 2 ? "Lines" + (current_sort_ascending ? asc_arrow : desc_arrow) : "Lines");
    file_tree->set_column_title(3, "Actions");

    TreeItem* root = file_tree->create_item();

    if (current_report.files.empty()) {
        return;
    }

    // Load User Thresholds
    CoverageSettings settings = SettingsGateway::load();
    float threshold_high = settings.threshold_high;
    float threshold_medium = settings.threshold_medium;

    // Prepare filter Regex if needed
    Ref<RegEx> glob_regex;
    bool use_regex = false;
    
    if (!current_filter.is_empty()) {
        if (current_filter.contains("*") || current_filter.contains("?")) {
            // Convert standard file globbing to robust RegEx pattern
            String regex_str = current_filter.replace(".", "\\.");
            regex_str = regex_str.replace("**", "<<GLOBSTAR>>");
            regex_str = regex_str.replace("*", "[^/]*");
            regex_str = regex_str.replace("<<GLOBSTAR>>", ".*");
            regex_str = regex_str.replace("?", ".");
            regex_str = "^" + regex_str + "$";

            glob_regex = RegEx::create_from_string(regex_str);
            use_regex = glob_regex.is_valid();
        }
    }

    std::vector<const FileCoverage*> sorted;
    for (const auto& kv : current_report.files) {
        String file_path = String(kv.second.source_file.c_str());
        
        // Filtering
        if (!current_filter.is_empty()) {
            if (use_regex) {
                if (glob_regex->search(file_path).is_null()) {
                    continue;
                }
            } else {
                // Substring match
                if (!file_path.containsn(current_filter)) { 
                    continue;
                }
            }
        }
        sorted.push_back(&kv.second);
    }

    // Sorting
    int sort_col = current_sort_column;
    bool asc = current_sort_ascending;

    std::sort(sorted.begin(), sorted.end(),
        [sort_col, asc](const FileCoverage* a, const FileCoverage* b) {
            bool result = false;
            if (sort_col == 0) { // File name
                result = a->source_file < b->source_file;
            } else if (sort_col == 1) { // Coverage %
                float pct_a = a->coverage_percent();
                float pct_b = b->coverage_percent();
                if (pct_a == pct_b) result = a->source_file < b->source_file;
                else result = pct_a < pct_b;
            } else if (sort_col == 2) { // Total Lines
                if (a->lines_found == b->lines_found) result = a->lines_hit < b->lines_hit;
                else result = a->lines_found < b->lines_found;
            }
            return asc ? result : !result;
        });

    // Try to get a file icon from the editor theme
    Ref<Texture2D> file_icon;
    if (has_theme_icon("Script", "EditorIcons")) {
        file_icon = get_theme_icon("Script", "EditorIcons");
    }

    for (const FileCoverage* file : sorted) {
        TreeItem* item = file_tree->create_item(root);
        item->set_text(0, String(file->source_file.c_str()));
        item->set_text(1, String::num(file->coverage_percent(), 1) + "%");
        item->set_text(2, String::num_int64(file->lines_hit) + "/" +
                          String::num_int64(file->lines_found));

        // Actions column
        if (file_icon.is_valid()) {
            item->add_button(3, file_icon, 0, false, "Open in editor");
        } else {
            item->set_text(3, "Open");
        }

        // Apply dynamically loaded coloring thresholds
        Color color;
        float pct = file->coverage_percent();
        if (pct >= threshold_high) {
            color = Color(0.2, 0.8, 0.2); // Green
        } else if (pct >= threshold_medium) {
            color = Color(0.9, 0.7, 0.1); // Yellow
        } else {
            color = Color(0.9, 0.2, 0.2); // Red
        }
        item->set_custom_color(1, color);
        item->set_metadata(0, String(file->source_file.c_str()));
    }
}

void CoveragePanel::clear_data() {
    current_report = LcovReport(); // Reset report cache
    if (summary_label) {
        summary_label->set_text("No coverage data.");
    }
    if (file_tree) {
        file_tree->clear();
    }
    if (filter_edit) {
        filter_edit->clear();
        current_filter = "";
    }
}

} // namespace godot