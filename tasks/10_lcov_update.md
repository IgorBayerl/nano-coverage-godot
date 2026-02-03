# Task: Update LCOV Writer for Merged Data

## Context Files
- `native/src/runtime/lcov_writer.cpp`
- `native/src/config/settings_gateway.h`

## Goal
Make LCOV generation data-driven (from a merged snapshot) and configurable.

## Instructions
1.  **Update `write_lcov_report` signature**:
    * Accept a `snapshot` object (from Task 9) instead of reading global state.
    * Accept `CoverageSettings` (from Task 4).
2.  **Implement Logic**:
    * If `settings.use_absolute_source_paths` is true: Output absolute paths in `SF:` lines.
    * Else: Output `res://` paths.
    * Use `settings.lcov_filename` for the output file.
3.  **Test**:
    * Test both absolute and relative path generation based on the bool flag.

## Validation
* Run `python scripts/test.py`.