extends GdUnitTestSessionHook

func _init() -> void:
	super ("NanoCoverage", "Instrument memory and generate LCOV reports for GdUnit4 tests.")

func startup(session: GdUnitTestSession) -> GdUnitResult:
	print("\n[NanoCoverage] GdUnit4 Session Hook: Starting Memory Instrumentation...")
	session.send_message("Starting NanoCoverage Memory Instrumentation...")
	
	var api = CoverageApi.new()
	api.clear_coverage_data({"workspace_id": "gdunit4"})
	
	var bootstrapper = ProjectBootstrapper.new()
	bootstrapper.instrument_all_scripts()
	
	print("[NanoCoverage] Instrumentation complete. Yielding to GdUnit4 tests.\n")
	session.send_message("Instrumentation complete. Yielding to tests.")
	
	return GdUnitResult.success()

func shutdown(session: GdUnitTestSession) -> GdUnitResult:
	print("\n[NanoCoverage] GdUnit4 Session Hook: Tests complete. Generating Report...")
	session.send_message("NanoCoverage: Tests complete. Generating Report...")
	
	# 1. Force the runtime to dump hits to disk
	if Engine.has_singleton("NanoCoverage"):
		var nc = Engine.get_singleton("NanoCoverage")
		
		# Log the total hits directly from C++ memory
		var hits = nc.get_total_hit_count()
		print("[NanoCoverage] Total hits currently in memory: ", hits)
		session.send_message("NanoCoverage: Collected %d execution hits." % hits)
		
		# Tell the C++ backend exactly where to save the .covdata file
		# so that the LCOV reporter can find it.
		ProjectSettings.set_setting("nano_coverage/output_dir", "res://coverage-data/gdunit4/runs")
		ProjectSettings.set_setting("nano_coverage/output_name", "gdunit_session.covdata")
		
		nc.save_session()
		nc.reset()
	
	# 2. Generate the report
	var api = CoverageApi.new()
	var report_opts = {"workspace_id": "gdunit4"}
	var result = api.generate_coverage_report(report_opts)
	
	if result.has("status") and result.status == "ok":
		print("[NanoCoverage] Report generated successfully at: ", result.report_path)
		session.send_message("NanoCoverage LCOV Report generated at: %s" % result.report_path)
	else:
		var err = result.get("error", "Unknown")
		printerr("[NanoCoverage] Report generation failed: ", err)
		session.send_message("NanoCoverage LCOV Report generation failed: %s" % err)
		
	return GdUnitResult.success()