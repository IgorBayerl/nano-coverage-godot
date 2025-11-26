# Nano Coverage Godot

A GDExtension for Godot 4.3 that collects code coverage for GDScript at runtime.

## Description

This tool allows you to measure which parts of your GDScript code are executed during tests or gameplay. It uses the Godot Engine's profiling API to intercept script execution and generates a standard LCOV report (`lcov.info`) that can be used with various coverage visualization tools.

## How it Works

The extension registers a custom `EngineProfiler` named "coverage". When enabled, this profiler receives callbacks from the Godot engine for every executed script frame. It tracks the visited scripts and lines, and when finished, it serializes this data into the LCOV format.

## Setup Instructions

### Prerequisites
1. **Python**: Install Python 3.x.
2. **SCons**: Install via pip: `pip install scons`.
3. **C++ Compiler**:
   - Windows: Visual Studio Build Tools (MSVC) or MinGW (g++).
   - Linux/Mac: GCC or Clang.
4. **Godot 4.3**: Download the standard version.

### Installation
1. Clone the repository:
   ```bash
   git clone <repo_url>
   cd nano-coverage-godot
   ```

2. Run the setup script:
   ```bash
   python setup.py
   ```
   This will initialize submodules, install SCons, and build the extension.

### Manual Installation (Alternative)
1. Initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

2. Build the extension:
   ```bash
   scons platform=windows target=template_debug
   ```
   (Replace `windows` with `linux` or `macos` as needed).

## Usage

1. **Open your project**: Open the `demo` project or your own project where the extension is installed.

2. **Enable Coverage**:
   In your test runner or game script, enable the coverage profiler using `EngineDebugger`:

   ```gdscript
   # Start collecting coverage
   EngineDebugger.profiler_enable("coverage", true)
   ```

3. **Run your tests**: Execute the code you want to measure.

4. **Disable and Save**:
   Disable the profiler to stop collection and trigger the report generation:

   ```gdscript
   # Stop collecting and save report
   EngineDebugger.profiler_enable("coverage", false)
   ```

5. **View Report**:
   The LCOV report will be saved to `res://coverage/lcov.info`.

## Limitations

> [!WARNING]
> **Debugger Requirement**: The coverage collector relies on the `EngineProfiler` API, which in Godot 4.3 requires an active `ScriptDebugger`.

- **Editor Mode**: Works out of the box when running from the Godot Editor (F5), as the editor attaches a debugger.
- **Headless Mode**: Does **NOT** work in pure headless mode (`--headless`) unless a remote debugger is connected. To run in CI, you must either:
    - Run with a dummy debugger server and connect using `--remote-debug`.
    - Run using the editor binary (requires display server).
