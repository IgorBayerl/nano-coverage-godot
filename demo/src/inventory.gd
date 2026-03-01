class_name Inventory extends RefCounted

signal item_added(item_id: String, amount: int)
signal item_removed(item_id: String, amount: int)

var items: Dictionary = {}

func add_item(item_id: String, amount: int = 1) -> bool:
	if amount <= 0:
		return false
	
	if items.has(item_id):
		items[item_id] += amount
	else:
		items[item_id] = amount
		
	item_added.emit(item_id, amount)
	return true

func remove_item(item_id: String, amount: int = 1) -> bool:
	if not items.has(item_id):
		return false
		
	if amount <= 0:
		return false
		
	var current = items[item_id]
	if current < amount:
		return false
		
	items[item_id] -= amount
	if items[item_id] == 0:
		items.erase(item_id)
		
	item_removed.emit(item_id, amount)
	return true

func get_item_count(item_id: String) -> int:
	return items.get(item_id, 0)

func process_items_by_type(type: String) -> Array:
	var result = []
	for item_id in items.keys():
		# simulate determining type by prefix
		if item_id.begins_with(type):
			result.append(item_id)
	return result

func get_item_category(item_id: String) -> String:
	match item_id:
		"sword", "shield":
			return "equipment"
		"potion_health", "potion_mana":
			return "consumable"
		_:
			if item_id.begins_with("quest_"):
				return "quest"
			return "misc"
