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
	# Creates a temporary copy of the project with tracking code injected.
	# Returns: Dictionary { "output_path": String, "error": String }
	var instr_opts = {}
	var instr_result = _api.instrument_project(instr_opts)
	
	if instr_result.has("error"):
		printerr("Instrumentation failed: ", instr_result.error)
		return

	var temp_path = instr_result.output_path
	print("Project instrumented at: ", temp_path)

	# 2. EXECUTION
	# Runs the instrumented project.
	# Options:
	#   - output_path: (Required) Path from step 1
	#   - workspace_id: (Optional) ID for session merging (default: "default")
	#   - blocking: (Optional) If true, waits for game to close (default: false)
	#   - dry_run: (Optional) Returns command args without running (default: false)
	var run_opts = {
		"output_path": temp_path,
		"workspace_id": "integration_test",
		"blocking": true # We wait here so we can generate the report immediately after
	}
	
	print("Running game...")
	var run_result = _api.run_instrumented_project(run_opts)
	
	if run_result.has("error"):
		printerr("Run failed: ", run_result.error)
		return
		
	print("Game run finished. Run ID: ", run_result.run_id)

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