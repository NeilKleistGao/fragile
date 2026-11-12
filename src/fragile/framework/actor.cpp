#include "actor.h"

#include <godot_cpp/core/class_db.hpp>

namespace fragile::framework {

void FActor::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("serialize", "data"), &FActor::serialize);
	godot::ClassDB::bind_method(godot::D_METHOD("deserialize", "data"), &FActor::deserialize);
}

godot::Error FActor::serialize(godot::Dictionary data) const {
	if (data.has(s_serialization_local_position_x_name) || data.has(s_serialization_local_position_y_name)) {
		godot::print_error(godot::String("Cannot serialize actor: ") + get_name());
		return godot::Error::ERR_INVALID_DATA;
	}

	const auto local_position = get_position();
	data[s_serialization_local_position_x_name] = local_position.x;
	data[s_serialization_local_position_y_name] = local_position.y;

	return godot::Error::OK;
}

godot::Error FActor::deserialize(const godot::Dictionary& data) {
	if (!data.has(s_serialization_local_position_x_name) || !data.has(s_serialization_local_position_y_name)) {
		godot::print_error(godot::String("Cannot deserialize actor: ") + get_name());
		return godot::Error::ERR_INVALID_DATA;
	}

	set_position(godot::Vector2(data[s_serialization_local_position_x_name], data[s_serialization_local_position_y_name]));

	return godot::Error::OK;
}

}	 // namespace fragile::framework
