@tool
extends RefCounted
## GdUnit4 Integration via Session Hooks

const SETTING_HOOKS = "gdunit4/settings/test/test_hooks"
const HOOK_SCRIPT_PATH = "res://addons/nano_coverage_godot/integrations/gdunit_hook.gd"

var _nano_api: Object

func _init(nano_api: Object) -> void:
	_nano_api = nano_api

static func is_available() -> bool:
	return ProjectSettings.has_setting(SETTING_HOOKS) or FileAccess.file_exists("res://addons/gdUnit4/plugin.cfg")

func enable() -> void:
	# Add our hook to GdUnit4's registered hooks
	var hooks: Array = ProjectSettings.get_setting(SETTING_HOOKS, [])
	if not HOOK_SCRIPT_PATH in hooks:
		hooks.append(HOOK_SCRIPT_PATH)
		ProjectSettings.set_setting(SETTING_HOOKS, hooks)
		ProjectSettings.save()
	print("[NanoCoverage] GdUnit4 Integration Enabled (Session Hook Registered)")

func disable() -> void:
	if ProjectSettings.has_setting(SETTING_HOOKS):
		var hooks: Array = ProjectSettings.get_setting(SETTING_HOOKS, [])
		if HOOK_SCRIPT_PATH in hooks:
			hooks.erase(HOOK_SCRIPT_PATH)
			ProjectSettings.set_setting(SETTING_HOOKS, hooks)
			ProjectSettings.save()
	print("[NanoCoverage] GdUnit4 Integration Disabled")
