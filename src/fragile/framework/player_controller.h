#ifndef __FRAGILE_PLAYER_CONTROLLER_H__
#define __FRAGILE_PLAYER_CONTROLLER_H__

#include "controller.h"

namespace fragile::framework {

/**
 * @brief A controller that drives its pawn from one human player's input.
 *
 * Mirrors Unreal Engine's APlayerController: the link between a human
 * player and the game world. It translates the player's input into
 * actions on the pawn it possesses and, like every FInfo, lives outside
 * any level.
 *
 * FPlayerController itself adds no state and no behavior beyond
 * FController. Derived classes add the actual input handling.
 */
class FPlayerController : public FController {
	GDCLASS(FPlayerController, FController)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_PLAYER_CONTROLLER_H__
