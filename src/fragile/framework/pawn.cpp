
#include "pawn.h"

#include "controller.h"
#include "fragile/utils/register_helper.h"

namespace fragile::framework {

void FPawn::_bind_methods() {
	ADD_METHOD(FPawn, get_controller);
}

FController* FPawn::get_controller() const {
	return controller;
}

}	 // namespace fragile::framework
