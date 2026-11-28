#ifndef __FRAGILE_PAWN_H__
#define __FRAGILE_PAWN_H__

#include "fragile/framework/actor.h"
#include "fragile/framework/input_component.h"
#include "godot_cpp/core/gdvirtual.gen.inc"

namespace fragile::framework {

class FController;

/**
 * @brief Base class for actors that can be possessed by a controller.
 *
 * Mirrors Unreal Engine's APawn: the physical representation of a player or
 * an AI-controlled entity inside the game world. Like any FActor a pawn
 * lives in an FLevel and can be spawned there by class name; unlike a plain
 * actor it is meant to be driven by an FController that takes possession of
 * it.
 *
 * Never free a pawn from user code: its lifetime belongs to the
 * framework. The FLevel that holds the pawn frees it along with the rest
 * of its actors.
 *
 * Its only state is the possessing FController, reported by
 * get_controller(). Derived classes add the actual representation of
 * the entity.
 */
class FPawn : public FActor {
	GDCLASS(FPawn, FActor)

private:
	/**
	 * @brief Controller that currently possesses this pawn; @c nullptr when unpossessed.
	 *
	 * Set by FController::control() and cleared by
	 * FController::remove_control(). Never freed by the pawn.
	 */
	FController* controller{ nullptr };

protected:
	static void _bind_methods();

	GDVIRTUAL1(_setup_input_component, FInputComponent*);

public:
	// Clears the controller's back-reference when this pawn is torn down.
	void _notification(int p_what);

	/**
	 * @brief Returns the controller that possesses this pawn.
	 *
	 * @return The controlling FController set by FController::control, or
	 *         @c nullptr when this pawn is unpossessed.
	 */
	[[nodiscard]] FController* get_controller() const;

	// FController::control() writes controller directly; the friendship keeps
	// the accessor read-only for every other caller.
	friend class FController;

	void on_input_component_setup(FInputComponent* ic);
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_PAWN_H__
