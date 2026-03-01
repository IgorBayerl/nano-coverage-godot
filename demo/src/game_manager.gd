class_name GameManager extends Node

signal score_changed(new_score: int)
signal level_completed(level_name: String)

var current_score: int = 0
var active_enemies: Array[Node] = []
var level_data: Dictionary = {}

func _ready() -> void:
	# Initialization logic
	reset_game()

func reset_game() -> void:
	current_score = 0
	active_enemies.clear()
	level_data = {
		"level_1": {"unlocked": true, "high_score": 0},
		"level_2": {"unlocked": false, "high_score": 0}
	}

func add_score(points: int) -> void:
	if points > 0:
		current_score += points
		score_changed.emit(current_score)

func register_enemy(enemy: Node) -> void:
	if not active_enemies.has(enemy):
		active_enemies.append(enemy)

func enemy_defeated(enemy: Node, points_value: int) -> void:
	if active_enemies.has(enemy):
		active_enemies.erase(enemy)
		add_score(points_value)
		
		if active_enemies.is_empty():
			complete_level("current_level")

func complete_level(level_name: String) -> void:
	level_completed.emit(level_name)
	
	if level_data.has(level_name):
		var data = level_data[level_name]
		if current_score > data["high_score"]:
			data["high_score"] = current_score
