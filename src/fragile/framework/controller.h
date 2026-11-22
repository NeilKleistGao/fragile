#ifndef __FRAGILE_CONTROLLER_H__
#define __FRAGILE_CONTROLLER_H__

#include "actor.h"

namespace fragile::framework {

class FPawn;

/**
 * @brief Base class of controllers that possess and drive pawns.
 *
 * Mirrors Unreal Engine's AController: an actor responsible for
 * directing the actions of an FPawn instead of acting in the world
 * itself. Like every FActor it is a scene tree node that can be placed
 * or spawned in a level.
 *
 * FController is registered as abstract and cannot be instantiated
 * directly - use FPlayerController or another derived class. Possession
 * is its only state: control() takes possession of an FPawn - adopting
 * it as a child node - and remove_control() gives it up again. Derived
 * classes add the actual direction logic.
 */
class FController : public FActor {
	GDCLASS(FController, FActor)

private:
	FPawn* controlled_pawn{ nullptr };

protected:
	static void _bind_methods();

public:
	/**
	 * @brief Makes this controller possess and drive @p pawn.
	 *
	 * Both ends of the relation are linked - the pawn reports this
	 * controller from FPawn::get_controller() - and the pawn is adopted
	 * as a child node of this controller. A controller drives one pawn
	 * at a time: while it already controls a pawn the call is refused
	 * with an error, even for that same pawn - call remove_control()
	 * first. A pawn is possessed by one controller at a time:
	 * possessing an already possessed pawn releases it from its
	 * previous controller first. Nothing is freed.
	 *
	 * @param pawn Pawn to possess; @c nullptr is refused with an error.
	 */
	void control(FPawn* pawn);

	/**
	 * @brief Releases the currently controlled pawn.
	 *
	 * Clears both ends of the relation so the pawn reports no controller
	 * again, and removes the pawn from this controller's children: the
	 * returned node is unparented and belongs to the caller again, who
	 * must re-parent or free it. A pawn that does not report this
	 * controller back is left untouched: the broken relation is dropped
	 * and the inconsistency is reported with an error.
	 *
	 * @return The released pawn, or @c nullptr when this controller
	 *         controls nothing (a no-op) or the relation was
	 *         inconsistent.
	 */
	FPawn* remove_control();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_CONTROLLER_H__
