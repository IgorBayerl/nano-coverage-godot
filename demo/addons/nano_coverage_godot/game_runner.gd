extends SceneTree

func _init():
	print("[NanoCoverage Runner] Bootstrapping Main Game...")
	
	# 1. Instrument memory using the new C++ Bootstrapper
	var bootstrapper = ProjectBootstrapper.new()
	bootstrapper.instrument_all_scripts()
	
	# 2. Inject Runtime monitor Node into SceneTree
	var runtime = CoverageRuntime.new()
	root.add_child(runtime)
	
	# 2. Find the user's actual main scene
	var main_scene_path = ProjectSettings.get_setting("application/run/main_scene")
	if main_scene_path == null or main_scene_path.is_empty():
		printerr("[NanoCoverage Runner] No main scene defined in Project Settings!")
		quit(1)
		return
		
	print("[NanoCoverage Runner] Loading main scene: ", main_scene_path)
	
	# 3. Load and switch to it
	var main_scene = load(main_scene_path)
	var instance = main_scene.instantiate()
	root.add_child(instance)
	current_scene = instance
	
	print("[NanoCoverage Runner] Game launched successfully!")
