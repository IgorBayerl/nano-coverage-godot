def run_tests():
    print("=== Running GDExtension Tests ===")
    
    godot_bin = os.path.join("bin", "Godot_v4.3-stable_win64.exe")
    project_path = "demo"
    
    if not os.path.exists(godot_bin):
        print(f"Error: Godot binary not found at {godot_bin}")
        sys.exit(1)
        
    cmd = [godot_bin, "--headless", "--path", project_path, "--quit"]
    
    print(f"Executing: {' '.join(cmd)}")
    try:
        subprocess.check_call(cmd)
        print("\n=== Tests Passed! ===")
    except subprocess.CalledProcessError as e:
        print(f"\n=== Tests Failed with exit code {e.returncode} ===")
        sys.exit(e.returncode)

if __name__ == "__main__":
    run_tests()
