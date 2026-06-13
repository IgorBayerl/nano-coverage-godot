@tool
extends EditorPlugin

var _plugin_instance
var _integration_manager


func _enter_tree() -> void:
	# Instantiate C++ Plugin
	if not ClassDB.class_exists("NanoCoverageEditorPlugin"):
		NanoCoverageLogger.error("NanoCoverageEditorPlugin class not found! Is the GDExtension built for this platform?")
		return

	_plugin_instance = ClassDB.instantiate("NanoCoverageEditorPlugin")
	add_child(_plugin_instance)

	# Check CoverageApi
	if not ClassDB.class_exists("CoverageApi"):
		NanoCoverageLogger.error("CoverageApi class not found in ClassDB!")
		return

	var api = ClassDB.instantiate("CoverageApi")
	if not api:
		NanoCoverageLogger.error("Failed to instantiate CoverageApi.")
		return

	# Load Integration Manager Script
	var manager_path = "res://addons/nano_coverage_godot/integrations/integration_manager.gd"
	if not FileAccess.file_exists(manager_path):
		NanoCoverageLogger.error("Integration Manager file missing at: " + manager_path)
		return

	var manager_script = load(manager_path)
	if not manager_script:
		NanoCoverageLogger.error("Failed to load Integration Manager script.")
		return

	_integration_manager = manager_script.new(api)
	NanoCoverageLogger.info("Editor plugin ready.")


func _exit_tree() -> void:
	if _integration_manager:
		_integration_manager.clean_up()
		_integration_manager = null

	if _plugin_instance:
		remove_child(_plugin_instance)
		_plugin_instance.queue_free()
		_plugin_instance = null
