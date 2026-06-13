#pragma once
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class ProjectBootstrapper : public RefCounted {
    GDCLASS(ProjectBootstrapper, RefCounted)

   protected:
    static void _bind_methods();

   public:
    void instrument_all_scripts();
};

}  // namespace godot
