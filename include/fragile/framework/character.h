#ifndef __FRAGILE_CHARACTER_H__
#define __FRAGILE_CHARACTER_H__

#include "fragile/framework/pawn.h"
#include "fragile/global.h"

#include <godot_cpp/classes/animated_sprite2d.hpp>

namespace fragile::framework {

class FInputComponent;

/**
 * @brief Base class for pawns with a shared character representation.
 *
 * Mirrors Unreal Engine's ACharacter: a pawn specialized for entities that
 * walk, jump and respond to input - the usual base for the player's avatar
 * and for humanoid NPCs. Where FPawn is a bare controllable actor, the
 * character is the class meant to carry the mesh, movement and input
 * machinery shared by all such entities.
 *
 * FCharacter itself adds no state beyond FPawn: it builds a default sprite in
 * _ready() when none exists and locates the input component attached by its
 * controller. Derived classes add the actual representation.
 */
class FCharacter : public FPawn {
	GDCLASS(FCharacter, FPawn)

protected:
	static void _bind_methods();

public:
	void _ready() override;

	/**
	 * @brief Returns the input component attached to this character.
	 *
	 * FPlayerController parents its input component directly under the pawn
	 * it possesses, so this looks for the first FInputComponent child.
	 *
	 * @return The attached component, or @c nullptr when the character is not
	 *         possessed (or was possessed by a controller without one).
	 */
	[[nodiscard]] FInputComponent* get_input_component() const;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_CHARACTER_H__
