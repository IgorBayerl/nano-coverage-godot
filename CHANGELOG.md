# Changelog

## v0.4.0 — Standalone Coverage & GdUnit4 Hook Fix

### Fixed

- **GdUnit4 hook was unregistered on every editor shutdown.** The plugin's exit path called `disable()` on the integration, removing the session hook from `project.godot` whenever the editor closed. Any CLI/CI test run (no editor open to re-register it) silently produced no coverage. The hook registration is now persistent and only removed when the user turns the integration setting off.
- **GdUnit4 integration registered its hook in the wrong setting.** The integration wrote to `gdunit4/settings/test/test_hooks`, a key no GdUnit4 version reads. It now registers in `gdunit4/hooks/session_hooks` (the typed dictionary used by GdUnit4's `GdUnitTestSessionHookService`), so enabling the integration in a fresh project actually produces coverage reports. The stale key is removed automatically from projects that have it.

### Added

- **Standalone coverage loop (no test framework required).** Disk instrumentation now registers a `NanoCoverageRuntime` autoload (`autoload/NanoCoverageRuntime` in `project.godot`) that flushes execution hits to the data store when the game exits. Pressing "Instrument", playing the game normally, and closing it now produces coverage data without any integration. The autoload is removed on "Restore".
- **Auto report generation.** The editor watches the coverage data store; when a Play session writes new data and `nano_coverage/general/auto_generate_report` is enabled (default), the LCOV report is regenerated and the gutters/panel refresh automatically.
- **`ScriptScanner` C++ class**: shared `.gd` discovery and ignore-glob handling extracted from `ProjectBootstrapper` and `DiskInstrumenter` (which previously had duplicated copies), with 5 dedicated unit tests.
- **2 new C++ unit tests** covering runtime autoload registration/removal during instrument/restore.

### Changed

- **Integration setting renamed**: `nano_coverage/integrations/gdunit4/active` is now `nano_coverage/integrations/gdunit4`, so Project Settings shows a single "Gdunit4" checkbox under *Integrations* instead of a "Gdunit4" group with an "Active" item. Existing projects are migrated automatically (the old key's value is carried over and the key removed).
- **Unified log format.** All output (C++ and GDScript) now goes through the `[NanoCoverage]` logger; the mixed `NanoCoverage:` / raw `print()` patterns are gone, and errors/warnings carry `[ERROR]` / `[WARN]` tags.
- **Better diagnostics in the GdUnit4 hook**: logs the collected hit count, warns when zero hits were collected (pointing at ignore-pattern configuration), and reports the resolved report path or the exact generation error.
- **`compatibility_minimum` raised to Godot 4.4** (typed dictionaries are required for GdUnit4 hook registration).
- **Editor rescans the filesystem after instrumenting** (not just after restore), keeping open scripts in sync with the patched files on disk.
- Disk instrumenter tests are now confined to their test directory via ignore globs, so a failing assertion can no longer leave demo project scripts instrumented; tests also no longer persist `project.godot`.

## v0.3.0 — Disk Instrumentation

### Added

- **Disk instrumentation mode**: `.gd` files are now patched on disk *before* Godot loads them, replacing the old "Run Instrumented" process-spawning approach. This eliminates the state-loss issues caused by hot-patching already-loaded scripts.
- **Instrument / Restore toolbar**: A `CheckButton` toggle replaces the old "Run Instrumented" button. Pressing it instruments all project scripts to disk with full backup. A "Restore" button appears alongside it to revert files to their originals.
- **Manifest-based state tracking**: A `manifest.json` file in the backup directory (`res://.nano_coverage/` by default) records every instrumented file, its backup path, and a SHA-256 hash of the original. The editor recovers toggle state from this manifest on startup or after a crash.
- **Safety guards**:
  - Double-instrument protection: refuses to instrument if a manifest already exists.
  - Marker detection: each instrumented file gets a `# __NANO_COVERAGE_INSTRUMENTED__` header line. If a file already has this marker, the operation aborts to prevent backing up instrumented code.
  - Hash verification on restore: warns if a backup file's hash doesn't match the manifest, but still restores (best-effort).
  - Exit warning: logs a prominent warning if the editor closes while files are still instrumented.
- **`backup_dir` setting**: New project setting `nano_coverage/general/backup_dir` (default: `res://.nano_coverage`) controls where backups and the manifest are stored.
- **DiskInstrumenter class**: New `RefCounted` class registered at scene level, usable from both editor and GDScript.
- **7 new C++ unit tests** covering round-trip, double-instrument guard, marker detection, no-manifest restore, hash verification, manifest structure, and ignore pattern respect.

### Changed

- **Toolbar buttons hidden by default**: The Instrument, Generate Report, and Clear Data buttons are now hidden by default. Users enable them via Project Settings (`nano_coverage/ui/`).
- **Removed game process management**: The editor plugin no longer spawns a separate Godot process or polls its PID. The `game_pid`, `process_poll_timer`, and `_on_poll_game_process()` code has been removed.
- **Instrumenter improvements**: Updated the tree-sitter instrumentation engine with better handling of `else`/`elif` clause entry points and inline statement detection. Golden tests updated to match.
- **Rewriter improvements**: Enhanced insertion logic for structural branches.

### Known Issues

- **Manual game testing is not yet stable.** When using the disk instrumentation toggle to run your game manually (press Instrument, then press Play), coverage collection may be unreliable depending on scene structure and autoload timing. This is an active area of development.
- **GdUnit4 integration works correctly.** The memory-mode instrumentation path used by GdUnit4 (via `ProjectBootstrapper`) is unchanged and remains stable for automated test coverage.
- **The `game_runner.gd` script is no longer launched from the editor** but still exists for backwards compatibility with CLI usage.
- **Users should add `.nano_coverage/` to `.gitignore`** to avoid committing backup files and the manifest.

---

## v0.2.0 — Coverage Display

- Coverage gutters in the script editor
- Coverage metrics bottom panel with sorting and search
- LCOV file watcher with auto-refresh
- Configurable high/medium coverage thresholds
- Toolbar buttons with editor theme icons

## v0.1.0 — Initial Release

- Tree-sitter based GDScript instrumentation
- NanoCoverage runtime singleton for hit collection
- GdUnit4 session hook integration
- LCOV report generation
- Project settings for ignore patterns, directories, and addons
