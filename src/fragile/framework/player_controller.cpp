
#include "fragile/framework/player_controller.h"

#include "fragile/framework/pawn.h"

namespace fragile::framework {

void FPlayerController::_bind_methods() {
}

void FPlayerController::control(FPawn* pawn) {
	FController::control(pawn);

	if (controlled_pawn == nullptr) {
		return;
	}

	input_component = memnew(FInputComponent);
	controlled_pawn->add_child(input_component);
	controlled_pawn->on_input_component_setup(input_component);
}

void FPlayerController::remove_control() {
	if (controlled_pawn == nullptr) {
		return;
	}

	if (input_component->get_parent() == controlled_pawn) {
		controlled_pawn->remove_child(input_component);
	}
	else {
		print_error("inconsistent input component.");
	}

	input_component->queue_free();
	input_component = nullptr;

	FController::remove_control();
}

}	 // namespace fragile::framework
