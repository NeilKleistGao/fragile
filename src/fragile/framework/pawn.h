#ifndef __FRAGILE_PAWN_H__
#define __FRAGILE_PAWN_H__

#include "actor.h"

namespace fragile::framework {

/**
 * @brief Base class for actors that can be possessed by a controller.
 *
 * Mirrors Unreal Engine's APawn: the physical representation of a player or
 * an AI-controlled entity inside the game world. Like any FActor a pawn
 * lives in an FLevel and can be spawned there by class name; unlike a plain
 * actor it is meant to be driven by an FController that takes possession of
 * it.
 *
 * FPawn itself adds no state and no behavior beyond FActor. Derived
 * classes add the actual representation of the entity.
 */
class FPawn : public FActor {
	GDCLASS(FPawn, FActor)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_PAWN_H__
