extends SceneTree

func _init():
	NanoCoverageLogger.info("Runner: Bootstrapping main game...")

	# 1. Instrument memory using the C++ Bootstrapper
	var bootstrapper = ProjectBootstrapper.new()
	bootstrapper.instrument_all_scripts()

	# 2. Inject Runtime monitor Node into SceneTree
	var runtime = CoverageRuntime.new()
	root.add_child(runtime)

	# 3. Find the user's actual main scene
	var main_scene_path = ProjectSettings.get_setting("application/run/main_scene")
	if main_scene_path == null or main_scene_path.is_empty():
		NanoCoverageLogger.error("Runner: No main scene defined in Project Settings!")
		quit(1)
		return

	NanoCoverageLogger.info("Runner: Loading main scene: %s" % main_scene_path)

	# 4. Load and switch to it
	var main_scene = load(main_scene_path)
	var instance = main_scene.instantiate()
	root.add_child(instance)
	current_scene = instance

	NanoCoverageLogger.info("Runner: Game launched successfully.")
