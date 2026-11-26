#ifndef __FRAGILE_F_INFO_H__
#define __FRAGILE_F_INFO_H__

#include "fragile/global.h"

#include <godot_cpp/classes/node.hpp>

namespace fragile::framework {

/**
 * @brief Base class of all "utility" objects not placed in the game world.
 *
 * Mirrors Unreal Engine's AInfo: it carries game information (rules, global
 * state) outside the level's spatial layout. It is a plain Node, so it has no
 * transform and never takes part in 2D sorting, even when a level parents it
 * to own its lifetime. FInfo itself has no state.
 */
class FInfo : public Node {
	GDCLASS(FInfo, Node)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_F_INFO_H__
