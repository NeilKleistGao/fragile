#ifndef __FRAGILE_PLAYER_CONTROLLER_H__
#define __FRAGILE_PLAYER_CONTROLLER_H__

#include "fragile/framework/controller.h"
#include "fragile/framework/input_component.h"

namespace fragile::framework {

/**
 * @brief A controller that drives its pawn from one human player's input.
 *
 * Mirrors Unreal Engine's APlayerController: the link between a human
 * player and the game world. It translates the player's input into
 * actions on the pawn it possesses and, like every FInfo, lives outside
 * any level.
 *
 * The controller owns the input side of the possession: control() creates
 * an FInputComponent and parents it to the possessed pawn, while
 * remove_control() detaches and frees it again. Derived classes add the
 * actual action handlers.
 */
class FPlayerController : public FController {
	GDCLASS(FPlayerController, FController)

protected:
	static void _bind_methods();

	/**
	 * @brief Input component parented to the possessed pawn.
	 *
	 * @c nullptr before the first possession or after remove_control().
	 * Owned by this controller: it is detached and freed whenever
	 * possession ends, including when the controller itself is torn down.
	 */
	FInputComponent* input_component{ nullptr };

public:
	/**
	 * @brief Possesses @p pawn and attaches this controller's input component.
	 *
	 * Delegates the relation itself to FController::control() and then makes
	 * sure the input component exists and is parented to the pawn.
	 * Possessing another pawn reuses the existing component.
	 *
	 * @param pawn Pawn to possess; @c nullptr is refused by FController.
	 */
	void control(FPawn* pawn) override;

	/**
	 * @brief Releases the pawn and detaches (then frees) the input component.
	 *
	 * Safe to call when nothing is controlled: FController::_notification()
	 * invokes it during teardown even when no pawn was ever possessed.
	 */
	void remove_control() override;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_PLAYER_CONTROLLER_H__
