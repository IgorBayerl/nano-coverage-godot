extends Node

var _agent = null

func _enter_tree():
    # 1. Debugging: Check file structure
    var addon_root = "res://addons/nano_coverage_godot/"
    _debug_list_dir(addon_root)
    _debug_list_dir(addon_root + "bin/")

    # 2. Attempt to connect
    if Engine.has_singleton("NanoCoverage"):
        _agent = Engine.get_singleton("NanoCoverage")
        print("NanoCoverage: Runtime connected successfully.")
    else:
        printerr("NanoCoverage: FATAL - C++ Singleton 'NanoCoverage' not found.")
        print("NanoCoverage: Debugging ClassDB...")
        # Check if the class is registered at all (even if singleton is missing)
        if ClassDB.class_exists("NanoCoverage"):
            print("NanoCoverage: Class 'NanoCoverage' exists in ClassDB (Extension loaded, but Singleton missing?)")
        else:
            printerr("NanoCoverage: Class 'NanoCoverage' NOT found in ClassDB (GDExtension failed to load)")

func _debug_list_dir(path: String):
    var dir = DirAccess.open(path)
    if dir:
        print("NanoCoverage: Listing contents of ", path)
        dir.list_dir_begin()
        var file_name = dir.get_next()
        while file_name != "":
            if not dir.current_is_dir():
                print("  [FILE] ", file_name)
            file_name = dir.get_next()
        dir.list_dir_end()
    else:
        printerr("NanoCoverage: Could not open directory: ", path)

func hit(file_path: String, line: int) -> void:
    if _agent:
        _agent.hit(file_path, line)
    
func _notification(what):
    if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_EXIT_TREE:
        if _agent:
            _agent.save_report("res://coverage.lcov")