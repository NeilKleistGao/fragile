
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
	if (controlled_pawn == nullptr) {
		return;
	}

	// The pawn may already have cleared its side of the relation while it is
	// being torn down; only clear it when it still points here.
	if (controlled_pawn->controller == this) {
		controlled_pawn->controller = nullptr;
	}

	controlled_pawn = nullptr;
}

void FController::_notification(int p_what) {
	if (p_what != NOTIFICATION_PREDELETE) {
		return;
	}

	remove_control();
}

}	 // namespace fragile::framework
