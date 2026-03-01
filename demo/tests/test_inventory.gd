class_name TestInventory extends GdUnitTestSuite

func test_add_item():
	var inv = Inventory.new()
	assert_bool(inv.add_item("sword", 1)).is_true()
	assert_int(inv.get_item_count("sword")).is_equal(1)
	
	# Add existing
	assert_bool(inv.add_item("sword", 2)).is_true()
	assert_int(inv.get_item_count("sword")).is_equal(3)
	
	# Add invalid amount
	assert_bool(inv.add_item("shield", -1)).is_false()

func test_remove_item():
	var inv = Inventory.new()
	inv.add_item("potion", 5)
	
	# Remove valid amount
	assert_bool(inv.remove_item("potion", 2)).is_true()
	assert_int(inv.get_item_count("potion")).is_equal(3)
	
	# Remove more than exists
	assert_bool(inv.remove_item("potion", 10)).is_false()
	
	# Remove exact amount (should erase)
	assert_bool(inv.remove_item("potion", 3)).is_true()
	assert_int(inv.get_item_count("potion")).is_equal(0)
	
	# Remove non-existent
	assert_bool(inv.remove_item("sword", 1)).is_false()

func test_process_items_by_type():
	var inv = Inventory.new()
	inv.add_item("weapon_sword")
	inv.add_item("weapon_axe")
	inv.add_item("armor_chest")
	
	var weapons = inv.process_items_by_type("weapon_")
	# Contains exactly does not care about order but expects exactly these elements
	assert_array(weapons).contains_exactly(["weapon_sword", "weapon_axe"])

func test_get_item_category():
	var inv = Inventory.new()
	assert_str(inv.get_item_category("sword")).is_equal("equipment")
	assert_str(inv.get_item_category("potion_mana")).is_equal("consumable")
	assert_str(inv.get_item_category("quest_amulet")).is_equal("quest")
	assert_str(inv.get_item_category("junk")).is_equal("misc")
