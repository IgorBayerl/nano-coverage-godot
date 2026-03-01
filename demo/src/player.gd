class_name DemoPlayer extends CharacterBody2D

signal health_changed(new_health: int)
signal player_died()

@export var max_health: int = 100
var current_health: int = 100

const SPEED = 300.0
const JUMP_VELOCITY = -400.0

# Get the gravity from the project settings to be synced with RigidBody nodes.
var gravity: float = ProjectSettings.get_setting("physics/2d/default_gravity")

enum State {IDLE, RUN, JUMP, FALL, DEAD}
var current_state: State = State.IDLE

func _ready() -> void:
	current_health = max_health

func _physics_process(delta: float) -> void:
	if current_state == State.DEAD:
		return
		
	# Add the gravity.
	if not is_on_floor():
		velocity.y += gravity * delta

	# Handle Jump.
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = JUMP_VELOCITY

	# Get the input direction and handle the movement/deceleration.
	# As good practice, you should replace UI actions with custom gameplay actions.
	var direction := Input.get_axis("ui_left", "ui_right")
	if direction:
		velocity.x = direction * SPEED
	else:
		velocity.x = move_toward(velocity.x, 0, SPEED)

	move_and_slide()
	_update_state()

func _update_state() -> void:
	if current_state == State.DEAD:
		return
		
	if not is_on_floor():
		if velocity.y < 0:
			current_state = State.JUMP
		else:
			current_state = State.FALL
	else:
		if abs(velocity.x) > 0.1:
			current_state = State.RUN
		else:
			current_state = State.IDLE

func take_damage(amount: int) -> void:
	if current_state == State.DEAD:
		return
		
	current_health -= amount
	# Ensure current_health isn't below 0 and above max
	if current_health < 0:
		current_health = 0
	elif current_health > max_health:
		current_health = max_health
	
	health_changed.emit(current_health)
	
	if current_health == 0:
		die()

func heal(amount: int) -> void:
	if current_state == State.DEAD:
		return
		
	current_health += amount
	if current_health > max_health:
		current_health = max_health
		
	health_changed.emit(current_health)

func die() -> void:
	current_state = State.DEAD
	player_died.emit()
