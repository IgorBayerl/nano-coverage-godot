#ifndef NANO_COVERAGE_TEST_MAIN_H
#define NANO_COVERAGE_TEST_MAIN_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

class NanoCoverageTestRunner : public Node {
    GDCLASS(NanoCoverageTestRunner, Node);  // Fixed: Added semicolon

   protected:
    static void _bind_methods();

   public:
    // Runs all registered C++ unit tests. Returns 0 on success, non-zero on failure.
    int run_all_tests();
};

}  // namespace godot

#endif  // NANO_COVERAGE_TEST_MAIN_H