extends Node

var _api: Object = null

# Safely fetches the C++ singleton dynamically exactly when needed
func _get_api() -> Object:
	if _api != null:
		return _api
		
	# Checks common names you might have used in register_types.cpp
	for singleton_name in ["CoverageCollector", "NanoCoverage", "NanoCoverageGodot", "CoverageApi"]:
		if Engine.has_singleton(singleton_name):
			_api = Engine.get_singleton(singleton_name)
			break
			
	return _api

func hit(file_path: String, line: int) -> void:
	var api = _get_api()
	if api != null:
		api.hit(file_path, line)

func _notification(what: int) -> void:
	# Handle application exit to flush data to disk
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_EXIT_TREE:
		var api = _get_api()
		if api != null and api.has_method("save_session"):
			api.save_session()