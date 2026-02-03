# Task: Centralize ProjectSettings Keys

## Context Files
- `native/src/editor/plugin.cpp` (Current location of hardcoded strings)
- `native/src/config/settings_keys.h` (New file)

## Goal
Remove magic strings for ProjectSettings keys and move them to a shared header file.

## Instructions
1.  **Create `native/src/config/settings_keys.h`**.
2.  **Define Constants** for the following keys:
    * `nano_coverage/general/temp_directory` (Existing)
    * `nano_coverage/paths/temp_dir`
    * `nano_coverage/paths/report_dir`
    * `nano_coverage/paths/data_store_dir`
    * `nano_coverage/report/lcov_filename`
    * `nano_coverage/report/use_absolute_source_paths`
    * `nano_coverage/ui/show_all_buttons`
    * `nano_coverage/ui/show_run_instrumented_button`
    * `nano_coverage/ui/show_generate_report_button`
    * `nano_coverage/ui/show_clear_data_button`
3.  **Refactor `native/src/editor/plugin.cpp`**: Replace all raw string literals matching the above with the new constants.
4.  **Test:** Add a trivial test (or compilation check) to ensure `settings_keys.h` can be included and constants are valid strings.

## Validation
* Compile the project.
* Run `python scripts/test.py`.
* Grep the codebase to ensure no raw `"nano_coverage/..."` strings remain in `plugin.cpp`.