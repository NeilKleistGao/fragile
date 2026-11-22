
#include "controller.h"

#include "pawn.h"

namespace fragile::framework {

void FController::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("control", "pawn"), &FController::control);
	godot::ClassDB::bind_method(godot::D_METHOD("remove_control"), &FController::remove_control);
}

void FController::control(FPawn* pawn) {
	ERR_FAIL_COND_MSG(pawn == nullptr, "Cannot control a null pawn.");
	ERR_FAIL_COND_MSG(controlled_pawn != nullptr, "This controller has already controlled a pawn.");

	if (FController* controller = pawn->get_controller(); controller != nullptr) {
		controller->remove_control();
	}

	controlled_pawn = pawn;
	controlled_pawn->controller = this;
	add_child(controlled_pawn);
}

FPawn* FController::remove_control() {
	FPawn* pawn = controlled_pawn;
	controlled_pawn = nullptr;

	if (pawn != nullptr) {
		if (pawn->controller != this) {
			godot::print_error("Inconsistent control relation: the controlled pawn does not report this controller.");
			return nullptr;
		}

		pawn->controller = nullptr;
		remove_child(pawn);
	}

	return pawn;
}

}	 // namespace fragile::framework
