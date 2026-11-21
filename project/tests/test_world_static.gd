extends SceneTree
# Unit tests for FWorld's static method binding
# (src/fragile/framework/world.cpp::_bind_methods).
#
# Run from the repository root:
#   <godot> --headless --path project -s res://tests/test_world_static.gd
#
# Exits 0 and prints WORLD_STATIC_TEST_PASS when every check passes,
# otherwise exits 1.

var failures: int = 0


func check(cond: bool, what: String) -> void:
	if cond:
		print("  ok: " + what)
	else:
		failures += 1
		print("  FAIL: " + what)


func _init() -> void:
	test_registration()
	# The world singleton can only be observed with a live scene tree
	# (see _process below).


func _process(_delta: float) -> bool:
	test_get_current_world()

	if failures == 0:
		print("WORLD_STATIC_TEST_PASS")
	else:
		print("WORLD_STATIC_TEST_FAIL: " + str(failures))
	quit(0 if failures == 0 else 1)
	return true


## get_current_world is bound to ClassDB as a static method of FWorld:
## it shows up in the method list with METHOD_FLAG_STATIC and is
## callable on the class itself.
func test_registration() -> void:
	print("== registration ==")
	check(ClassDB.class_has_method("FWorld", "get_current_world"), "get_current_world is exposed on FWorld")
	var is_static: bool = false
	for method in ClassDB.class_get_method_list("FWorld"):
		if String(method["name"]) == "get_current_world":
			is_static = (method["flags"] & METHOD_FLAG_STATIC) != 0
	check(is_static, "get_current_world is bound as a static method")


## The static call reports the one world that currently owns the scene
## tree, and null while there is none.
func test_get_current_world() -> void:
	print("== get_current_world ==")
	check(FWorld.get_current_world() == null, "no world is current while the tree has none")

	var world = ClassDB.instantiate("FWorld")
	root.add_child(world) # entering the tree makes it the current world
	check(FWorld.get_current_world() == world, "the world in the tree is the current world")

	root.remove_child(world) # leaving the tree clears the current world
	check(FWorld.get_current_world() == null, "leaving the tree clears the current world")

	root.add_child(world)
	world.free() # freeing the world clears the current world too
	check(FWorld.get_current_world() == null, "freeing the world clears the current world")
