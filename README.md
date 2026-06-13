# Nano Coverage Godot

**Nano Coverage** is a simple, powerful code coverage tool for Godot 4.

It helps you see exactly which parts of your GDScript code are executed when you run your game or your unit tests. This is essential for ensuring your tests actually test your code, helping you find bugs before they happen.

---

## 🚧 Status & Roadmap

**Current State:** Alpha / Developer Preview.  
**Note:** We currently do not have automated builds (CI/CD). You must build the plugin from source using the instructions below.

### ✅ Current Features
* **Accurate Coverage:** Uses advanced parsing (Tree-sitter) to ignore comments and empty lines, counting only real code.
* **In-Editor Display:** Coverage gutters in the script editor and a metrics bottom panel, refreshed automatically when the report changes.
* **Standalone Mode:** Instrument your project, press Play, close the game — a coverage report is generated automatically. No test framework required.
* **Test Framework Integrations:** Built-in GdUnit4 integration via session hooks; more integrations planned.
* **LCOV Support:** Generates standard `lcov.info` files compatible with popular tools like Coveralls, Codecov, or IDE extensions.
* **Session Merging:** Run your game multiple times; the tool accumulates coverage data across all sessions.
* **Headless Support:** Works from the command line for automated testing environments.

### 🚀 Planned Features
* **More Integrations:** GUT and other Godot testing frameworks.
* **Godot Asset Library:** We plan to publish this to the Asset Library for one-click installation.
* **Automated Builds:** Pre-compiled binaries for Windows, Linux, and macOS.

---

## ⚙️ How It Works

Nano Coverage has two instrumentation modes:

**Disk mode (standalone — no test framework needed):**
1.  **Instrument:** Toggling "Instrument" in the toolbar patches your `.gd` files on disk (with full backups and a manifest), *before* Godot loads them. Stateful scripts and autoloads behave normally because nothing is hot-reloaded.
2.  **Play:** Run your game with the regular Play button. Invisible trackers record which lines are hit, and a temporary autoload flushes the data when the game exits.
3.  **Report:** The editor detects the new data and generates the LCOV report automatically (or click "Generate Report"). Toggle "Instrument" off to restore your original files.

**Memory mode (used by integrations like GdUnit4):**
1.  When a test session starts, scripts are instrumented in memory inside the test runner process — nothing on disk changes.
2.  When the session ends, the hook flushes the hits and generates the report automatically.

---

## 🛠️ Building from Source

Since pre-compiled binaries aren't available yet, you need to build the plugin yourself. We have automated this process with Python scripts.

### Prerequisites
* **Python 3.10+**
* **Git**
* **C++ Compiler:**
    * *Windows:* MinGW (recommended) or MSVC.
    * *Linux:* GCC or Clang.
    * *macOS:* Xcode Command Line Tools.

### Build Steps

1.  **Clone the Repository:**
    ```bash
    git clone https://github.com/IgorBayerl/nano-coverage-godot.git
    cd nano-coverage-godot
    ```

2.  **Setup Environment:**
    Downloads the Godot engine (for testing), Google Test, and installs SCons.
    ```bash
    python scripts/setup.py
    ```

3.  **Compile:**
    Compiles the C++ code into a Godot GDExtension.
    ```bash
    python scripts/build.py
    ```

4.  *(Optional)* **Test the Build:**
    Runs the included Unit Tests to ensure everything is working.
    ```bash
    python scripts/test.py
    ```

---

## 📥 Installing in Your Project

Once you have built the project, you can install it into your own Godot game.

1.  Locate the **`addons`** folder inside `godot_project/` (generated after the build).
2.  Copy the `addons/nano_coverage_godot` folder into your own project's root directory.
    * *Your project structure should look like:* `res://addons/nano_coverage_godot/`
3.  Open your project in Godot.
4.  Go to **Project > Project Settings > Plugins**.
5.  Find **NanoCoverage** and check the **Enable** box.

---

## 🎮 Usage

### Standalone (play your game manually)

1.  **Instrument:**
    Enable the toolbar buttons via `nano_coverage/ui/` in Project Settings, then toggle **"Instrument"** in the main editor toolbar. Your scripts are patched on disk with full backups (add `.nano_coverage/` to `.gitignore`).
2.  **Play:**
    Run your game normally and exercise the code you want covered.
3.  **Exit:**
    Close the game window. Coverage data is flushed automatically.
4.  **View Results:**
    With `auto_generate_report` enabled (default), the report regenerates by itself and the editor gutters/panel refresh. `lcov.info` lands in `res://coverage-report/` and works with any LCOV viewer (like the "Coverage Gutters" extension for VS Code).
5.  **Restore:**
    Toggle **"Instrument"** off to restore your original scripts before committing.

### With GdUnit4

1.  Enable the integration: **Project > Project Settings > Nano Coverage > Integrations > Gdunit4**.
2.  Run your test suite as usual. The session hook instruments scripts in memory, and the LCOV report is generated when the test session finishes.

---

## 🤖 API & Integrations

Nano Coverage exposes a `CoverageApi` class to GDScript.

If you are building a test runner integration or a CI pipeline, you can use this API to programmatically:
* Instrument the project (in memory via `ProjectBootstrapper`, or on disk via `DiskInstrumenter`).
* Run the instrumented instance.
* Generate reports without using the Editor UI.

Refer to `addons/nano_coverage_godot/coverage_api_example.gd` or the C++ source for the exact method signatures.

---

## 🔧 Configuration

You can customize the tool via **Project > Project Settings > Nano Coverage**

| Setting                            | Description                                                      | Default                 |
|:-----------------------------------|:-----------------------------------------------------------------|:------------------------|
| **General / Report Dir**           | Where the `lcov.info` file is saved.                             | `res://coverage-report` |
| **General / Data Store Dir**       | Where raw coverage data is kept.                                 | `res://coverage-data`   |
| **General / Backup Dir**           | Where disk-instrumentation backups and the manifest are stored.  | `res://.nano_coverage`  |
| **General / Ignore Paths**         | Glob patterns excluded from instrumentation.                     | `**/*_test.gd`          |
| **General / Auto Generate Report** | Regenerate the report when a Play session writes new data.       | `true`                  |
| **General / Watch Lcov File**      | Refresh gutters/panel when the LCOV file changes.                | `true`                  |
| **UI / ...**                       | Show/hide the individual toolbar buttons.                        | hidden                  |
| **Integrations / Gdunit4**         | Enable the GdUnit4 session hook.                                 | `false`                 |

---