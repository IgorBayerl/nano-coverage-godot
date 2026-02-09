# Nano Coverage Godot

**Nano Coverage** is a simple, powerful code coverage tool for Godot 4.

It helps you see exactly which parts of your GDScript code are executed when you run your game or your unit tests. This is essential for ensuring your tests actually test your code, helping you find bugs before they happen.

---

## 🚧 Status & Roadmap

**Current State:** Alpha / Developer Preview.  
**Note:** We currently do not have automated builds (CI/CD). You must build the plugin from source using the instructions below.

### ✅ Current Features
* **Accurate Coverage:** Uses advanced parsing (Tree-sitter) to ignore comments and empty lines, counting only real code.
* **Simple UI:** Adds "Run Instrumented" and "Generate Report" buttons directly to the Godot Editor toolbar.
* **LCOV Support:** Generates standard `lcov.info` files compatible with popular tools like Coveralls, Codecov, or IDE extensions.
* **Session Merging:** Run your game multiple times; the tool accumulates coverage data across all sessions.
* **Headless Support:** Works from the command line for automated testing environments.

### 🚀 Planned Features
* **In editor Reports:** Visualization of covered and uncovered lines inside of code editor.
* **Godot Asset Library:** We plan to publish this to the Asset Library for one-click installation.
* **Automated Builds:** Pre-compiled binaries for Windows, Linux, and macOS.

---

## ⚙️ How It Works

1.  **Instrumentation:** When you click "Run Instrumented", Nano Coverage creates a temporary copy of your project. It injects invisible trackers into your GDScript code.
2.  **Execution:** Your game runs normally. As you play or run tests, the trackers record which lines are hit.
3.  **Reporting:** When you finish, you click "Generate Report" to combine the data into a LCOV file.

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

1.  **Run Instrumented:**
    Click the **"Run Instrumented"** button in the main editor toolbar. This launches your game with coverage tracking enabled.
2.  **Play/Test:**
    Play your game or run your unit test suite.
3.  **Exit:**
    Close the game window.
4.  **Generate Report:**
    Click the **"Generate Report"** button in the toolbar.
5.  **View Results:**
    A file named `lcov.info` will be created in your project folder (usually under `coverage_report/`). You can view this using any LCOV viewer (like the "Coverage Gutters" extension for VS Code).

---

## 🤖 API & Integrations

Nano Coverage exposes a `CoverageApi` class to GDScript.

If you are building a test runner (like **GdUnit4**) or a CI pipeline, you can use this API to programmatically:
* Instrument the project.
* Run the instrumented instance.
* Generate reports without using the Editor UI.

Refer to `addons/nano_coverage_godot/coverage_api_example.gd` or the C++ source for the exact method signatures.

---

## 🔧 Configuration

You can customize the tool via **Project > Project Settings > Nano Coverage**

| Setting                         | Description                                                 | Default                 |
|:--------------------------------|:------------------------------------------------------------|:------------------------|
| **Paths / Report Dir**          | Where the `lcov.info` file is saved.                        | `res://coverage-report` |
| **Paths / Data Store**          | Where raw coverage data is kept.                            | `res://coverage-data`   |
| **Report / Use Absolute Paths** | If true, uses full system paths in reports (useful for CI). | `false`                 |
| **UI / Show All Buttons**       | Hides/Shows the toolbar buttons.                            | `true`                  |

---