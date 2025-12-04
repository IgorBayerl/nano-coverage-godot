# Nano Coverage Godot

**Nano Coverage Godot** is a high-performance, GDExtension-based code coverage tool designed specifically for Godot 4. It provides accurate line coverage for GDScript by instrumenting code at the AST level using [tree-sitter](https://tree-sitter.github.io/tree-sitter/), ensuring robust and reliable reporting without modifying your original project files.

## 🚀 Features

*   **GDScript Line Coverage**: Accurate tracking of executed lines in your GDScript files.
*   **Non-Invasive**: Instruments a temporary copy of your project, keeping your source code untouched.
*   **LCOV Output**: Generates standard `lcov.info` files compatible with most coverage visualization tools (e.g., Coveralls, Codecov, VS Code extensions).
*   **Editor Integration**: Adds a simple "Run Instrumented" button directly to the Godot editor toolbar.
*   **High Performance**: Implemented as a C++ GDExtension for minimal runtime overhead.
*   **Tree-sitter Powered**: Uses robust parsing for accurate instrumentation, avoiding fragile regex-based approaches.

## 🛠️ Prerequisites

To build and use Nano Coverage Godot, you need:

*   **Godot 4.1+**
*   **Python 3.x** (for build scripts)
*   **C++ Compiler**:
    *   **Windows**: Visual Studio (MSVC) or MinGW-w64.
    *   **Linux**: GCC or Clang (`build-essential`).
    *   **macOS**: Xcode Command Line Tools.

## 📦 Setup & Build

We provide automated scripts to set up your environment and build the extension.

1.  **Clone the repository**:
    ```bash
    git clone --recursive https://github.com/IgorBayerl/nano-coverage-godot.git
    cd nano-coverage-godot
    ```

2.  **Run the setup script**:
    This will initialize submodules, check for dependencies (installing SCons if needed), and verify your compiler.
    ```bash
    python setup.py
    ```

3.  **Build the extension**:
    This compiles the C++ GDExtension and places the binaries in the Godot project folder.
    ```bash
    python build.py
    ```
    *   *Optional arguments*: `--platform [windows|linux|macos]`, `--target [template_debug|template_release]`, `--clean`.

## 🎮 Usage

1.  Open the `godot_project` folder in the Godot Editor.
2.  Go to **Project > Project Settings > Plugins**.
3.  Enable **NanoCoverageGodot**.
4.  A new button **"Run Instrumented"** will appear in the main toolbar (usually near the Play buttons).
5.  Click **"Run Instrumented"** to start your game with coverage tracking enabled.
6.  Interact with your game/run tests.
7.  Exit the game.
8.  An `lcov.info` file will be generated in the project root (or specified output directory).

## 🏗️ Architecture

The system consists of three main components:

1.  **Editor Plugin (C++)**: Adds the UI integration and manages the temporary project build process.
2.  **Instrumentation Layer**: Uses `tree-sitter` to parse GDScript files and inject `NanoCoverage.hit()` calls at executable lines.
3.  **Runtime Collector**: A high-performance singleton that records hits in memory and flushes them to an LCOV file upon application exit.

## 📄 License

MIT License
