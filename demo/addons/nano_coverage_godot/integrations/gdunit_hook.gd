extends GdUnitTestSessionHook
# Note: In a real environment, GdUnitTestSessionHook is provided by GdUnit4.

func _init() -> void:
	pass

# Called by GdUnit4 inside the test runner process BEFORE tests start
func startup(_session: GdUnitTestSession) -> GdUnitResult:
	print("\n[NanoCoverage] GdUnit4 Session Hook: Starting Memory Instrumentation...")
	# 1. Clear old data for this workspace
	var api = CoverageApi.new()
	api.clear_coverage_data({"workspace_id": "gdunit4"})
	
	# 2. Instrument all code in memory
	NanoCoverageBootstrap.instrument_all_scripts()
	
	print("[NanoCoverage] Instrumentation complete. Yielding to GdUnit4 tests.\n")
	return GdUnitResult.success()

# Called by GdUnit4 AFTER tests finish
func shutdown(_session: GdUnitTestSession) -> GdUnitResult:
	print("\n[NanoCoverage] GdUnit4 Session Hook: Tests complete. Generating Report...")
	
	# 1. Force the runtime to dump hits to disk NOW (before process dies)
	if Engine.has_singleton("NanoCoverage"):
		Engine.get_singleton("NanoCoverage").save_session()
	
	# 2. Generate the report
	var api = CoverageApi.new()
	var report_opts = {"workspace_id": "gdunit4"}
	var result = api.generate_coverage_report(report_opts)
	
	if result.has("status") and result.status == "ok":
		print("[NanoCoverage] Report generated at: ", result.report_path)
	else:
		printerr("[NanoCoverage] Report generation failed: ", result.get("error", "Unknown"))
		
	return GdUnitResult.success()
