@tool
extends EditorPlugin

var _plugin_instance
var _integration_manager


func _enter_tree() -> void:
	print("NanoCoverage Plugin Loading")

	# Instantiate C++ Plugin
	if not ClassDB.class_exists("NanoCoverageEditorPlugin"):
		printerr("Error: NanoCoverageEditorPlugin class not found!")
		return

	_plugin_instance = ClassDB.instantiate("NanoCoverageEditorPlugin")
	add_child(_plugin_instance)
	print("C++ Editor Plugin instantiated")

	# Check CoverageApi
	if not ClassDB.class_exists("CoverageApi"):
		printerr("Error: CoverageApi class not found in ClassDB!")
		return

	var api = ClassDB.instantiate("CoverageApi")
	if not api:
		printerr("Error: Failed to instantiate CoverageApi")
		return
	
	print("CoverageApi instantiated")

	# Load Integration Manager Script
	var manager_path = "res://addons/nano_coverage_godot/integrations/integration_manager.gd"
	if not FileAccess.file_exists(manager_path):
		printerr("Error: Integration Manager file missing at: ", manager_path)
		return

	var manager_script = load(manager_path)
	if not manager_script:
		printerr("Error: Failed to load Integration Manager script")
		return

	# Initialize Manager
	_integration_manager = manager_script.new(api)
	print("Integration Manager initialized")


func _exit_tree() -> void:
	if _integration_manager:
		_integration_manager.clean_up()
		_integration_manager = null

	if _plugin_instance:
		remove_child(_plugin_instance)
		_plugin_instance.queue_free()
		_plugin_instance = null
