#pragma once

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include "../reporting/lcov_parser.h"

namespace godot {

class CoveragePanel : public VBoxContainer {
    GDCLASS(CoveragePanel, VBoxContainer)

private:
    LineEdit* filter_edit = nullptr;
    Label* summary_label = nullptr;
    Tree* file_tree = nullptr;

    LcovReport current_report;
    String current_filter = "";
    
    // Default to sorting by Column 0 (File Name), Ascending (A-Z)
    int current_sort_column = 0;
    bool current_sort_ascending = true;

    void _on_item_activated();
    void _on_button_clicked(TreeItem* item, int32_t column, int32_t id, int32_t mouse_button);
    void _on_filter_changed(const String& text);
    void _on_column_title_clicked(int32_t column, int32_t mouse_button);
    void _open_file(TreeItem* item);
    void _populate_tree();

protected:
    static void _bind_methods();

public:
    void setup();
    void update_data(const LcovReport& report);
    void clear_data();
};

} // namespace godot
