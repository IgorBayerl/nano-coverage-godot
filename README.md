# Nano Coverage Godot

A GDExtension-based code coverage tool for Godot 4. It instruments GDScript at the AST level using [tree-sitter](https://tree-sitter.github.io/tree-sitter/) to generate `lcov.info` reports.

## Compatibility Strategy

This project enforces a **"Build Old, Run New"** workflow to maintain ABI compatibility across Godot 4.x versions.

  * **Build:** Links against **Godot 4.3** bindings. This ensures the binary works on 4.3, 4.4, 4.5, and future 4.x releases without recompilation.
  * **Test:** The setup script automatically downloads the latest stable Godot version (currently 4.5) for the editor environment.

## Prerequisites

You only need **Python**, **Git**, and a **C++ Compiler** installed manually. The setup script handles the rest (SCons, Godot Editor, and dependencies).

  * **Python 3.10+**
  * **Git**
  * **C++ Compiler:**
      * **Windows:** MinGW-w64 (recommended) or MSVC.
      * **Linux:** GCC or Clang.
      * **macOS:** Xcode Command Line Tools.

## Quick Start

```bash
# 1. Clone
git clone https://github.com/IgorBayerl/nano-coverage-godot.git
cd nano-coverage-godot

# 2. Setup (Downloads Godot 4.5, installs SCons, pins dependencies)
python setup.py

# 3. Build (Compiles the GDExtension)
python build.py

# 4. Run Editor (Opens the included project)
python dev.py
```

## Developer Scripts

### `setup.py`

Initializes the environment.

  * Updates and pins git submodules (forces `godot-cpp` to `4.3-stable`).
  * Installs `scons` via pip if missing.
  * Downloads the Godot Editor binary for local testing.

### `build.py`

Wrapper around SCons.

  * **Windows:** Auto-detects MinGW if MSVC is missing.
  * **Arguments:**
      * `--target`: `template_debug` (default) or `template_release`.
      * `--clean`: Cleans build artifacts.
      * `--platform`: Forces specific platform (usually auto-detected).

### `dev.py`

Launcher for the test project.

  * **Default:** Opens the project in the Editor.
  * `--game`: Runs the project directly (no editor).
  * `--verbose`: Enables stdout logging.
  * `--top`: Keeps the window always on top.

## Usage

1.  Enable **NanoCoverageGodot** in **Project \> Project Settings \> Plugins**.
2.  Click **"Run Instrumented"** in the main toolbar.
3.  Run your tests or gameplay loop.
4.  Exit the application.
5.  Coverage data is written to `coverage.lcov` in the project root.

## Architecture

1.  **AST Instrumentation:**
      * Uses `tree-sitter-gdscript` to parse source code.
      * Identifies executable statements (skips comments, whitespace, class decls).
      * Injects `NanoCoverage.hit(file_hash, line)` calls into a temporary script copy.
2.  **GDExtension Backend:**
      * **Collector:** A C++ singleton that aggregates hit counts in memory for minimal overhead.
      * **EditorPlugin:** Manages the UI and the temporary instrumented run configuration.
3.  **Output:**
      * Standard LCOV format for integration with Coveralls, Codecov, or local viewers.
