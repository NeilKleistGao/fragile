
#include "fragile/framework/pawn.h"

#include "fragile/framework/controller.h"
#include "fragile/utils/register_helper.h"

namespace fragile::framework {

void FPawn::_bind_methods() {
	ADD_METHOD(FPawn, get_controller);
}

FController* FPawn::get_controller() const {
	return controller;
}

void FPawn::_notification(int p_what) {
	if (p_what != NOTIFICATION_PREDELETE) {
		return;
	}

	// The scene tree frees the pawn; drop both ends of the possession relation
	// so the controller cannot reach this node afterwards.
	if (controller != nullptr) {
		controller->remove_control();
		controller = nullptr;
	}
}

}	 // namespace fragile::framework
