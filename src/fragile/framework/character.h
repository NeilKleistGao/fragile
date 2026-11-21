#ifndef __FRAGILE_CHARACTER_H__
#define __FRAGILE_CHARACTER_H__

#include "pawn.h"

namespace fragile::framework {

/**
 * @brief Base class for pawns with a shared character representation.
 *
 * Mirrors Unreal Engine's ACharacter: a pawn specialized for entities that
 * walk, jump and respond to input - the usual base for the player's avatar
 * and for humanoid NPCs. Where FPawn is a bare controllable actor, the
 * character is the class meant to carry the mesh, movement and input
 * machinery shared by all such entities.
 *
 * FCharacter itself adds no state and no behavior beyond FPawn. Derived
 * classes add the actual representation.
 */
class FCharacter : public FPawn {
	GDCLASS(FCharacter, FPawn)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_CHARACTER_H__
