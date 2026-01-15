extends Sprite2D

const SPEED = 400.0

func _ready() -> void:
	print("Player initialized at ", position)
	# This line should run once

func _process(delta: float) -> void:
	# This function runs every frame
	# 1. Handle Movement
	var input := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	
	if input != Vector2.ZERO:
		# These lines run only when keys are pressed
		position += input * SPEED * delta
		rotation += 5.0 * delta
	else:
		# These lines run when IDLE
		rotation = lerp(rotation, 0.0, 10.0 * delta)

func _input(event: InputEvent) -> void:
	# This runs on any input event (mouse, keyboard, etc)
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		# Check if the click is inside the sprite's local rect
		if get_rect().has_point(to_local(event.position)):
			_change_color()

func _change_color() -> void:
	# This is a custom function, runs only on click
	modulate = Color(randf(), randf(), randf())
	print("Sprite clicked! New Color: ", modulate)
