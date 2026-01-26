extends Node

# C++ singleton reference here.
var _agent

func _enter_tree() -> void:
	_agent = Engine.get_singleton("NanoCoverage")

func hit(file: String, line: int) -> void:
	_agent.hit(file, line)

func _notification(what: int) -> void:
	# Handle application exit to flush data to disk
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_EXIT_TREE:
		if _agent:
			var path = ProjectSettings.get_setting("nano_coverage/output_path", "res://coverage.lcov")
			_agent.save_report(path)