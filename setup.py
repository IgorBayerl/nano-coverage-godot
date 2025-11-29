import subprocess
import sys
import os

def run_command(command):
    print(f"Running: {command}")
    subprocess.check_call(command, shell=True)

def main():
    print("=== Nano Coverage Setup (with Tree-sitter) ===")

    # 1. Initialize Git Submodules
    if os.path.exists(".git"):
        # We add tree-sitter and the gdscript grammar
        # Note: We pin specific versions/branches if needed, but HEAD is usually fine for these.
        cmds = [
            "git submodule add https://github.com/tree-sitter/tree-sitter.git thirdparty/tree-sitter",
            "git submodule add https://github.com/PrestonKnopp/tree-sitter-gdscript.git thirdparty/tree-sitter-gdscript",
            "git submodule update --init --recursive"
        ]
        for cmd in cmds:
            try:
                run_command(cmd)
            except:
                pass # Ignore if already exists

    # 2. Build
    print("\n[Building Extension]")
    platform = "windows" if sys.platform == "win32" else "linux"
    if sys.platform == "darwin": platform = "macos"
    
    run_command(f"{sys.executable} -m SCons platform={platform} target=template_debug")

if __name__ == "__main__":
    main()
