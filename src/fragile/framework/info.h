#ifndef __FRAGILE_F_INFO_H__
#define __FRAGILE_F_INFO_H__

#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node.hpp>

namespace fragile::framework {

class FInfo : public godot::Node, IMPLEMENTS(ISerializable) {
	GDCLASS(FInfo, godot::Node)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_F_INFO_H__
