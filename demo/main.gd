extends Node

func _ready():
	print("Starting coverage test...")
	var coverage = CoverageCollector.new()
	add_child(coverage)
	coverage.start_coverage()
	coverage.stop_coverage()
	print("Coverage test complete")
	get_tree().quit()
