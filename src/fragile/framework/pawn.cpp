
#include "pawn.h"

#include "controller.h"

namespace fragile::framework {

void FPawn::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("get_controller"), &FPawn::get_controller);
}

FController* FPawn::get_controller() const {
	return controller;
}

}	 // namespace fragile::framework
