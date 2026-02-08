@tool
extends EditorPlugin

var plugin_instance

func _enter_tree():
	# Instantiate the C++ class
	plugin_instance = NanoCoverageEditorPlugin.new()
	add_child(plugin_instance)

func _exit_tree():
	if plugin_instance:
		remove_child(plugin_instance)
		plugin_instance.queue_free()
		plugin_instance = null
