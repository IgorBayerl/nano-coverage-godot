#pragma once
#include <godot_cpp/classes/node.hpp>

namespace godot {

class CoverageRuntime : public Node {
    GDCLASS(CoverageRuntime, Node)

private:
    bool is_saved = false;
    void flush_data();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    void _ready() override;
    void _exit_tree() override;
    
    // Slot for GdUnit4 integration
    void _on_gdunit_event(const Variant& event);
};

} // namespace godot
