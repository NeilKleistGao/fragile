#ifndef __FRAGILE_FRAMEWORK_ACTOR_H__
#define __FRAGILE_FRAMEWORK_ACTOR_H__

#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node2d.hpp>

namespace fragile::framework {

class FActor : public godot::Node2D, IMPLEMENTS(ISerializable) {
	GDCLASS(FActor, godot::Node2D)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_ACTOR_H__
