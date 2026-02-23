extends SceneTree

func _init():
	print("[NanoCoverage] Bootstrapping Main Game...")
	
	# 1. Instrument everything in memory
	NanoCoverageBootstrap.instrument_all_scripts()
	
	# 2. Find the user's actual main scene
	var main_scene_path = ProjectSettings.get_setting("application/run/main_scene")
	if main_scene_path == null or main_scene_path.is_empty():
		printerr("[NanoCoverage] No main scene defined in Project Settings!")
		quit(1)
		return
		
	# 3. Load and switch to it
	var main_scene = load(main_scene_path)
	var instance = main_scene.instantiate()
	root.add_child(instance)
	current_scene = instance
