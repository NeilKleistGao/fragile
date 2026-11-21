#ifndef __FRAGILE_CONTROLLER_H__
#define __FRAGILE_CONTROLLER_H__

#include "info.h"

namespace fragile::framework {

/**
 * @brief Base class of controllers that possess and drive pawns.
 *
 * Mirrors Unreal Engine's AController: a non-physical actor responsible
 * for directing the actions of an FPawn instead of existing in the world
 * itself. Like every FInfo it is not part of any level.
 *
 * FController is registered as abstract and cannot be instantiated
 * directly - use FPlayerController or another derived class. It adds no
 * state and no behavior beyond FInfo. Derived classes add the actual
 * direction logic.
 */
class FController : public FInfo {
	GDCLASS(FController, FInfo)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_CONTROLLER_H__
