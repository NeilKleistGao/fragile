#ifndef __FRAGILE_CONTROLLER_H__
#define __FRAGILE_CONTROLLER_H__

#include "fragile/global.h"
#include "info.h"

namespace fragile::framework {

class FPawn;

/**
 * @brief Base class of controllers that possess and drive pawns.
 *
 * Mirrors Unreal Engine's AController: a non-physical actor responsible
 * for directing the actions of an FPawn instead of existing in the world
 * itself. Like every FInfo it is a plain Node with no transform and never
 * appears in the level's spatial layout, even though the level it belongs to
 * parents it to own its lifetime.
 *
 * FController is registered as abstract and cannot be instantiated
 * directly - use FPlayerController or another derived class. Possession
 * is its only state: control() takes possession of an FPawn and
 * remove_control() gives it up again. Derived classes add the actual
 * direction logic.
 *
 * The possessed pawn is deliberately never mounted under the
 * controller as a child node: it must stay a child of its level so
 * that Y-sorting can order every pawn by its Y position. Mounting the
 * pawn under the controller would pull it out of the level's Y-sorted
 * children and break that ordering.
 */
class FController : public FInfo {
	GDCLASS(FController, FInfo)

private:
	/**
	 * @brief Pawn this controller possesses; @c nullptr when none is controlled.
	 *
	 * Set by control() and cleared by remove_control(). The pawn is not a
	 * child node of the controller (see the class note above).
	 */
	FPawn* controlled_pawn{ nullptr };

protected:
	static void _bind_methods();

public:
	// Clears the pawn's back-reference when this controller is torn down.
	void _notification(int p_what);

	/**
	 * @brief Makes this controller possess and drive @p pawn.
	 *
	 * Both ends of the relation are linked: the pawn reports this
	 * controller from FPawn::get_controller(). A controller drives one
	 * pawn at a time - possessing @p pawn releases the previously
	 * controlled pawn - and a pawn is possessed by one controller at a
	 * time - possessing an already possessed pawn releases it from its
	 * previous controller first. Neither node is moved or freed - the
	 * pawn stays under its level for Y-sorting (see the class note
	 * above).
	 *
	 * @param pawn Pawn to possess; @c nullptr is refused with an error.
	 */
	void control(FPawn* pawn);

	/**
	 * @brief Releases the currently controlled pawn.
	 *
	 * Clears both ends of the relation so the pawn reports no controller
	 * again. A no-op when this controller controls nothing.
	 */
	void remove_control();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_CONTROLLER_H__
