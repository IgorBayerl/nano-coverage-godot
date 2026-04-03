#pragma once

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include "../reporting/lcov_parser.h"

namespace godot {

class CoveragePanel : public VBoxContainer {
    GDCLASS(CoveragePanel, VBoxContainer)

private:
    Label* summary_label = nullptr;
    Tree* file_tree = nullptr;

    void _on_item_activated();
    void _on_button_clicked(TreeItem* item, int32_t column, int32_t id, int32_t mouse_button);
    void _open_file(TreeItem* item);

protected:
    static void _bind_methods();

public:
    void setup();
    void update_data(const LcovReport& report);
    void clear_data();
};

} // namespace godot
