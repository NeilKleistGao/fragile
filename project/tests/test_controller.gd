extends SceneTree
# Unit tests for the FController / FPawn possession feature
# (src/fragile/framework/controller.h, src/fragile/framework/pawn.h).
#
# Run from the repository root:
#   <godot> --headless --path project -s res://tests/test_controller.gd
#
# Exits 0 and prints CONTROLLER_TEST_PASS when every check passes,
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
	test_possession()
	test_steal()


func _process(_delta: float) -> bool:
	if failures == 0:
		print("CONTROLLER_TEST_PASS")
	else:
		print("CONTROLLER_TEST_FAIL: " + str(failures))
	quit(0 if failures == 0 else 1)
	return true


## FController is registered but abstract - concrete controllers like
## FPlayerController are instantiable - and derives from FActor, so it
## is a scene tree node like every actor. The possession surface is
## control()/remove_control() on the controller, get_controller() on the
## pawn.
func test_registration() -> void:
	print("== registration ==")
	check(ClassDB.class_exists("FController"), "ClassDB knows FController")
	check(not ClassDB.can_instantiate("FController"), "FController is abstract")
	check(ClassDB.can_instantiate("FPlayerController"), "FPlayerController can be instantiated")
	check(ClassDB.is_parent_class("FController", "FActor"), "FController derives from FActor")
	check(not ClassDB.is_parent_class("FController", "FInfo"), "FController no longer derives from FInfo")
	check(ClassDB.is_parent_class("FPawn", "FActor"), "FPawn derives from FActor")

	var controller = ClassDB.instantiate("FPlayerController")
	check(controller.has_method("control"), "control is exposed to scripts")
	check(controller.has_method("remove_control"), "remove_control is exposed to scripts")
	controller.free()

	var pawn = ClassDB.instantiate("FPawn")
	check(pawn.has_method("get_controller"), "get_controller is exposed to scripts")
	check(pawn.get_controller() == null, "a fresh pawn is unpossessed")
	pawn.free()


## A controller possesses one pawn at a time: control() links both ends
## and adopts the pawn as a child node, remove_control() releases and
## unparents the pawn again. While the controller already holds a pawn,
## every further control() call is refused with an error and nothing
## changes.
func test_possession() -> void:
	print("== possession ==")
	var controller = ClassDB.instantiate("FPlayerController")
	var p1 = ClassDB.instantiate("FPawn")
	var p2 = ClassDB.instantiate("FPawn")

	controller.control(p1)
	check(p1.get_controller() == controller, "control() links the pawn to its controller")
	check(p1.get_parent() == controller, "control() adopts the pawn as a child of its controller")

	print("  (expected errors follow)")
	controller.control(p1) # the controller already holds a pawn: refused
	check(p1.get_controller() == controller, "possessing the same pawn again is refused")
	check(p1.get_parent() == controller, "a refused control() leaves the pawn where it is")

	controller.control(p2) # one controller drives one pawn at a time
	check(p2.get_controller() == null, "a second pawn is refused while one is held")
	check(p1.get_controller() == controller, "the held pawn is kept")

	controller.control(null)
	check(p1.get_controller() == controller, "control(null) is refused")

	var released = controller.remove_control()
	check(released == p1, "remove_control() returns the released pawn")
	check(p1.get_controller() == null, "remove_control() releases the pawn")
	check(p1.get_parent() == null, "remove_control() unparents the pawn")

	check(controller.remove_control() == null, "remove_control() without a pawn returns null")

	p1.free()
	p2.free()
	controller.free()


## One pawn is possessed by one controller at a time: taking over a
## possessed pawn releases it from its previous controller first and
## re-parents the pawn under the new controller.
func test_steal() -> void:
	print("== stealing ==")
	var c1 = ClassDB.instantiate("FPlayerController")
	var c2 = ClassDB.instantiate("FPlayerController")
	var pawn = ClassDB.instantiate("FPawn")

	c1.control(pawn)
	c2.control(pawn) # taken over without an explicit release
	check(pawn.get_controller() == c2, "control() steals a possessed pawn for the new controller")
	check(pawn.get_parent() == c2, "the stolen pawn is adopted by its new controller")
	check(c1.remove_control() == null, "the relieved controller holds nothing")

	c1.control(pawn) # the relieved controller can take it back
	check(pawn.get_controller() == c1, "the relieved controller can possess the pawn again")
	check(pawn.get_parent() == c1, "the pawn is re-adopted by the relieved controller")

	check(c2.remove_control() == null, "remove_control() of a relieved controller returns null")
	check(pawn.get_controller() == c1, "remove_control() of a relieved controller changes nothing")

	var released = c1.remove_control()
	check(released == pawn, "remove_control() hands the pawn back")
	check(pawn.get_controller() == null, "the pawn ends up released")
	check(pawn.get_parent() == null, "the pawn ends up unparented")

	pawn.free()
	c1.free()
	c2.free()
