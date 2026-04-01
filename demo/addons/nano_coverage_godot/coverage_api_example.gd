extends RefCounted
class_name NanoCoverageIntegration

# ==============================================================================
# NANO COVERAGE API EXAMPLE
# ==============================================================================
# This script demonstrates how to control Nano Coverage programmatically.
# Use this reference if you are building:
#   - A CI/CD pipeline script
#   - A custom test runner (e.g., GdUnit4 integration)
#   - A custom editor tool
# ==============================================================================

# The API class is registered automatically by the GDExtension.
# You can instantiate it directly.
var _api = CoverageApi.new()

func run_full_coverage_cycle():
	print("--- Starting Coverage Cycle ---")

	# 1. INSTRUMENTATION
	# Use ProjectBootstrapper to instrument all scripts in-memory.
	# This patches loaded GDScript sources with tracking calls.
	var bootstrapper = ProjectBootstrapper.new()
	bootstrapper.instrument_all_scripts()
	print("Project instrumented in-memory.")

	# 2. EXECUTION
	# At this point, run your game or tests normally.
	# Instrumented scripts will call NanoCoverage.hit() on every executed line.
	# When the game/tests finish, save the session:
	if Engine.has_singleton("NanoCoverage"):
		var nc = Engine.get_singleton("NanoCoverage")
		nc.save_session("integration_test")
		nc.reset()
		print("Session saved.")

	# 3. REPORT GENERATION
	# Merges data and produces the lcov.info file.
	# Options:
	#   - workspace_id: Must match the one used in step 2.
	var report_opts = {
		"workspace_id": "integration_test"
	}

	var report_result = _api.generate_coverage_report(report_opts)

	if report_result.has("report_path"):
		print("Success! Report generated at: ", report_result.report_path)
	else:
		printerr("Report generation failed.")

func clear_old_data():
	# Helper to wipe previous data for a specific workspace
	_api.clear_coverage_data({"workspace_id": "integration_test"})
