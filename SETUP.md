# Setup Instructions

## Prerequisites
1. **Python**: Install Python 3.x.
2. **SCons**: Install via pip: `pip install scons`.
3. **C++ Compiler**:
   - Windows: Visual Studio Build Tools (MSVC) or MinGW (g++).
   - Linux/Mac: GCC or Clang.
4. **Godot 4.3**: Download the standard version.

## Installation
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

## Manual Installation (Alternative)
1. Initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

## Building
1. Build the extension:
   ```bash
   scons platform=windows target=template_debug
   ```
   (Replace `windows` with `linux` or `macos` as needed).

## Running
1. Open `demo/project.godot` in Godot.
2. Run the project.
