@tool
extends RefCounted
## GdUnit4 Integration via Session Hooks
##
## Registers the NanoCoverage session hook with GdUnit4 so test runs are
## instrumented and produce an LCOV report automatically.

const HOOK_SCRIPT_PATH = "res://addons/nano_coverage_godot/integrations/gdunit_hook.gd"

## GdUnit4 stores session hooks in a typed dictionary (script path -> enabled).
const SETTING_SESSION_HOOKS = "gdunit4/hooks/session_hooks"
## Stale key written by old NanoCoverage versions; never read by GdUnit4.
## Kept only so we can clean it up from existing projects.
const SETTING_STALE_HOOKS = "gdunit4/settings/test/test_hooks"
## The session hook service ships with every GdUnit4 version we support.
const HOOK_SERVICE_SCRIPT = "res://addons/gdUnit4/src/core/hooks/GdUnitTestSessionHookService.gd"

var _nano_api: Object


func _init(nano_api: Object) -> void:
	_nano_api = nano_api


static func is_available() -> bool:
	return FileAccess.file_exists("res://addons/gdUnit4/plugin.cfg")


static func _supports_session_hooks() -> bool:
	return FileAccess.file_exists(HOOK_SERVICE_SCRIPT)


func enable() -> void:
	_remove_stale_hook_setting()

	if not _supports_session_hooks():
		NanoCoverageLogger.error("The installed GdUnit4 version has no session hook support. Update GdUnit4 to enable the integration.")
		return

	var existing: Dictionary = ProjectSettings.get_setting(SETTING_SESSION_HOOKS, {})
	if existing.get(HOOK_SCRIPT_PATH, false):
		NanoCoverageLogger.info("GdUnit4 integration active (session hook already registered).")
		return

	# GdUnit4 expects a typed Dictionary[String, bool]; rebuild to guarantee the type.
	var hooks: Dictionary[String, bool] = {}
	for key: String in existing.keys():
		hooks[key] = existing[key]
	hooks[HOOK_SCRIPT_PATH] = true

	ProjectSettings.set_setting(SETTING_SESSION_HOOKS, hooks)
	ProjectSettings.save()
	NanoCoverageLogger.info("GdUnit4 integration enabled (session hook registered).")


func disable() -> void:
	_remove_stale_hook_setting()

	if ProjectSettings.has_setting(SETTING_SESSION_HOOKS):
		var existing: Dictionary = ProjectSettings.get_setting(SETTING_SESSION_HOOKS, {})
		if existing.has(HOOK_SCRIPT_PATH):
			var hooks: Dictionary[String, bool] = {}
			for key: String in existing.keys():
				if key != HOOK_SCRIPT_PATH:
					hooks[key] = existing[key]
			ProjectSettings.set_setting(SETTING_SESSION_HOOKS, hooks)
			ProjectSettings.save()

	NanoCoverageLogger.info("GdUnit4 integration disabled (session hook removed).")


func _remove_stale_hook_setting() -> void:
	if ProjectSettings.has_setting(SETTING_STALE_HOOKS):
		ProjectSettings.set_setting(SETTING_STALE_HOOKS, null)
		NanoCoverageLogger.info("Removed stale setting '%s'." % SETTING_STALE_HOOKS)
