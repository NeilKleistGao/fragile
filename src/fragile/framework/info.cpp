#include "info.h"

#include <godot_cpp/core/class_db.hpp>

namespace fragile::framework {

void FInfo::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("serialize", "data"), &FInfo::serialize);
	godot::ClassDB::bind_method(godot::D_METHOD("deserialize", "data"), &FInfo::deserialize);
}

godot::Error FInfo::serialize(godot::Dictionary) const {
	return godot::Error::OK;
}

godot::Error FInfo::deserialize(const godot::Dictionary&) {
	return godot::Error::OK;
}

}	 // namespace fragile::framework
