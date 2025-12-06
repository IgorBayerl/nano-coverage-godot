#ifndef NANO_COVERAGE_TEST_RUNNER_HPP
#define NANO_COVERAGE_TEST_RUNNER_HPP

#include <godot_cpp/classes/node.hpp>

namespace godot {

class NanoCoverageTestRunner : public Node {
    GDCLASS(NanoCoverageTestRunner, Node)

protected:
    static void _bind_methods();

public:
    // Runs all registered C++ unit tests. Returns the number of failures.
    int run_all_tests();
};

} // namespace godot

#endif // NANO_COVERAGE_TEST_RUNNER_HPP