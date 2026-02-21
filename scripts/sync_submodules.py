import configparser
import os
import subprocess
import sys
import shutil

# --- Colors for Output ---
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def log(msg, color=Colors.RESET):
    print(f"{color}{msg}{Colors.RESET}")

def run_git(args, cwd=None, ignore_error=False):
    """Runs a git command and returns (success, output)"""
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )
        return True, result.stdout.strip()
    except subprocess.CalledProcessError as e:
        if ignore_error:
            return False, e.stderr.strip()
        # Only log error if we are not ignoring it
        if not ignore_error: 
            log(f"Git Error: {' '.join(args)}\n{e.stderr}", Colors.RED)
        return False, e.stderr

def get_registered_submodules():
    """Returns a list of submodule paths currently known to Git index"""
    success, output = run_git(["submodule", "status"], ignore_error=True)
    if not success: return []
    
    paths = []
    for line in output.splitlines():
        # format: -hash path (version)
        parts = line.split()
        if len(parts) >= 2:
            paths.append(parts[1])
    return paths

def clean_config_value(value):
    """Removes inline comments starting with ; or # and trims whitespace"""
    if not value: return None
    return value.split(';')[0].split('#')[0].strip()

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    gitmodules_path = os.path.join(root_dir, ".gitmodules")

    log("=== Submodule Sync: '.gitmodules' is Source of Truth ===", Colors.HEADER)

    if not os.path.exists(gitmodules_path):
        log("Error: .gitmodules file not found!", Colors.RED)
        sys.exit(1)

    # 1. Parse .gitmodules
    config = configparser.ConfigParser()
    config.read(gitmodules_path)

    modules_in_file = []

    # 2. Iterate through modules defined in the text file
    for section in config.sections():
        if not section.startswith('submodule "'):
            continue
            
        # FIX: Clean the values to remove comments
        path = clean_config_value(config[section]['path'])
        url = clean_config_value(config[section]['url'])
        branch = clean_config_value(config[section].get('branch', None))
        
        modules_in_file.append(path)
        full_path = os.path.join(root_dir, path)

        log(f"Checking: {path}...", Colors.BLUE)

        # Robust check: Is it in the git index?
        is_in_index = False
        status_ok, _ = run_git(["ls-files", "--error-unmatch", path], ignore_error=True)
        if status_ok:
            is_in_index = True
        
        # If folder exists but it's not in index, it might be a failed previous attempt
        if os.path.exists(full_path) and not is_in_index:
            log(f"   -> Folder exists but not in Git Index. Cleaning up...", Colors.YELLOW)
            try:
                # Force remove the folder to allow fresh add
                if os.path.isdir(full_path):
                    shutil.rmtree(full_path)
                else:
                    os.remove(full_path)
            except Exception as e:
                log(f"   -> Failed to delete {full_path}: {e}", Colors.RED)

        if not is_in_index:
            log(f"   -> Missing in Git Index. Adding now...", Colors.YELLOW)
            
            # Ensure parent dir exists
            parent_dir = os.path.dirname(full_path)
            if not os.path.exists(parent_dir):
                os.makedirs(parent_dir)

            # Construct add command
            cmd = ["submodule", "add", "--force"]
            if branch:
                cmd.extend(["-b", branch])
            cmd.extend([url, path])

            success, _ = run_git(cmd, cwd=root_dir)
            if success:
                log(f"   -> Successfully added {path}", Colors.GREEN)
            else:
                log(f"   -> Failed to add {path}.", Colors.RED)
        else:
            log(f"   -> Present. Syncing URL...", Colors.GREEN)
            # Force sync the URL in case the text file changed
            run_git(["submodule", "sync", path], cwd=root_dir)
            
            # If branch changed in text file, we might need to update that config manually
            if branch:
                run_git(["config", f"submodule.{path}.branch", branch], cwd=root_dir)

    # 3. Cleanup: Check for "Ghost" submodules (In git index, but NOT in file)
    registered_paths = get_registered_submodules()
    for reg_path in registered_paths:
        if reg_path not in modules_in_file:
            log(f"\n[WARNING] Found submodule in Git Index NOT in .gitmodules: '{reg_path}'", Colors.RED)
            log(f"   -> To remove it, run: git rm {reg_path}", Colors.YELLOW)

    # 4. Final Update
    log("\nRunning final update...", Colors.BLUE)
    run_git(["submodule", "update", "--init", "--recursive"], cwd=root_dir)
    log("\nDone! Git internal state now matches .gitmodules.", Colors.GREEN)

if __name__ == "__main__":
    main()