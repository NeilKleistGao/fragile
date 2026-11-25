
#include "controller.h"

#include "fragile/utils/register_helper.h"
#include "pawn.h"

namespace fragile::framework {

void FController::_bind_methods() {
	ADD_METHOD(FController, control, "pawn");
	ADD_METHOD(FController, remove_control);
}

void FController::control(FPawn* pawn) {
	ERR_FAIL_COND_MSG(pawn == nullptr, "Cannot control a null pawn.");
	remove_control();	 // one pawn at a time: release the previously controlled one
	if (FController* controller = pawn->get_controller(); controller != nullptr) {
		controller->remove_control();
	}

	controlled_pawn = pawn;
	controlled_pawn->controller = this;
}

void FController::remove_control() {
	if (controlled_pawn != nullptr) {
		ERR_FAIL_COND_MSG(controlled_pawn->controller != this, "Inconsistent control relation.");
		controlled_pawn->controller = nullptr;
	}

	controlled_pawn = nullptr;
}

}	 // namespace fragile::framework
