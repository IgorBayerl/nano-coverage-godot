# Contributing

## Development Workflow

### Directory Structure
- `src/`: C++ source code.
- `godot-cpp/`: Godot C++ bindings (submodule).
- `demo/`: Godot project for testing.

### Adding New Classes
1. Create `.h` and `.cpp` files in `src/`.
2. Inherit from `godot::Node` or other Godot classes.
3. Register the class in `src/register_types.cpp`.
4. Rebuild using `scons`.

### Testing
- The `demo` project contains a `Main` scene that instantiates the extension class.
- Run the demo project to verify changes.
