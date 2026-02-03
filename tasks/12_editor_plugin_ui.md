# Task: Wire EditorPlugin to API

## Context Files
- `native/src/editor/plugin.cpp`
- `native/src/api/coverage_api.h`

## Goal
Replace the old plugin logic with calls to `CoverageApi` and add visibility controls.

## Instructions
1.  **Refactor Plugin**:
    * Remove direct dependencies on `TempProjectBuilder` or `LcovWriter`.
    * Instantiate `CoverageApi` to perform actions.
2.  **Add Toolbar Buttons**:
    * "Run Instrumented"
    * "Generate Report"
    * "Clear Data"
3.  **Implement Visibility**:
    * Subscribe to `ProjectSettings::settings_changed`.
    * Check `CoverageSettings` (show_all_buttons, etc.).
    * Show/Hide buttons dynamically based on settings.

## Validation
* Run `python scripts/test.py`.
* (Manual) Verify buttons appear/disappear when settings are toggled in the Editor.