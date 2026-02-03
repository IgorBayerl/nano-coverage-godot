# Task: Refactor TempProjectBuilder

## Context Files
- `native/src/editor/temp_builder.cpp`
- `native/src/editor/temp_builder.h`

## Goal
Break the monolithic `create_temp_project()` function into small, readable helper methods. **DO NOT change logic or behavior.**

## Instructions
1.  **Analyze `create_temp_project`** and extract the following private methods:
    * `collect_files()`: Scans the source directory.
    * `copy_or_link_file()`: Handles the file duplication logic.
    * `instrument_if_needed()`: Decides if a file needs `Instrumenter`.
    * `sync_godot_cache()`: Handles `.godot` folder logic.
    * `patch_project_settings()`: Modifies the `project.godot` config.
2.  **Reassemble**: Rewrite `create_temp_project()` to be a linear sequence of calls to these new helpers.
3.  **Check**: Ensure `sanity_check` or error handling remains intact.

## Validation
* **Strict requirement:** The existing `TempProjectBuilderTest` must pass without modification.
* Run `python scripts/test.py`.