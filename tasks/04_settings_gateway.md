# Task: Implement SettingsGateway

## Context Files
- `native/src/config/settings_keys.h`
- `native/src/config/settings_gateway.h` (New)
- `native/src/config/settings_gateway.cpp` (New)
- `native/src/editor/plugin.cpp`

## Goal
Create a typed configuration loader and a centralized place to register ProjectSettings metadata.

## Instructions
1.  **Create `SettingsGateway` class**:
    * Define a struct `CoverageSettings` containing fields for all keys defined in Task 3 (paths as strings, UI options as bools).
2.  **Implement `register_settings()`**:
    * Use `ProjectSettings::add_property_info` to register keys.
    * **Important:** Use `PROPERTY_HINT_GLOBAL_DIR` for directory paths (temp_dir, report_dir, store_dir).
    * Set reasonable defaults.
3.  **Implement `load()`**:
    * Read from `ProjectSettings::get_setting()`.
    * Return the populated `CoverageSettings` struct.
4.  **Update EditorPlugin**:
    * In `_enter_tree()`, replace inline settings registration with `SettingsGateway::register_settings()`.
5.  **Unit Test**:
    * Test that `load()` returns the defaults when no settings are changed.

## Validation
* Run `python scripts/test.py`.