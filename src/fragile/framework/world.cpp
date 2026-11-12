#include "world.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>

namespace fragile::framework {

void FWorld::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("serialize", "data"), &FWorld::serialize);
	godot::ClassDB::bind_method(godot::D_METHOD("deserialize", "data"), &FWorld::deserialize);
	godot::ClassDB::bind_method(godot::D_METHOD("open_level", "level_name"), &FWorld::open_level);
}

godot::Error FWorld::serialize(godot::Dictionary data) const {
	if (data.has(s_serialization_level_name)) {
	serialize_fail:
		godot::print_error(godot::String("Cannot serialize world: ") + get_name());
		return godot::Error::ERR_INVALID_DATA;
	}

	if (current_level != nullptr) {
		auto level_data = godot::Dictionary();
		level_data[s_serialization_type_name] = current_level->get_class();
		if (current_level->serialize(level_data) != godot::Error::OK) {
			goto serialize_fail;
		}

		data[s_serialization_level_name] = level_data;
	}

	return godot::Error::OK;
}

godot::Error FWorld::deserialize(const godot::Dictionary& data) {
	if (data.has(s_serialization_level_name)) {
		if (data[s_serialization_level_name].get_type() != godot::Variant::DICTIONARY) {
		deserialize_fail:
			godot::print_error(godot::String("Cannot deserialize world: ") + get_name());
			return godot::Error::ERR_INVALID_DATA;
		}

		const godot::Dictionary d = data[s_serialization_level_name];
		if (!d.has(s_serialization_type_name)) {
			goto deserialize_fail;	// TODO: more error information?
		}

		const auto type = d[s_serialization_type_name].get_type();
		if (type != godot::Variant::STRING && type != godot::Variant::STRING_NAME) {
			goto deserialize_fail;	// TODO: more error information?
		}

		const auto* old_level = current_level;
		open_level(d[s_serialization_type_name]);
		if (current_level == nullptr || current_level == old_level || current_level->deserialize(d) != godot::Error::OK) {
			goto deserialize_fail;	// TODO: more error information?
		}
	}

	return godot::Error::OK;
}

void FWorld::open_level(const godot::StringName& level_name) {
	if (godot::ClassDB::class_exists(level_name) && godot::ClassDB::can_instantiate(level_name)) {
		Object* object = godot::ClassDB::instantiate(level_name);
		if (auto* level = cast_to<FLevel>(object); level != nullptr) {
			if (current_level != nullptr) {
				remove_child(current_level);
				current_level->queue_free();
			}

			current_level = level;
			add_child(current_level);
			return;
		}

		if (object != nullptr && cast_to<godot::RefCounted>(object) == nullptr) {
			godot::memdelete(object);
		}
	}

	godot::print_error(godot::String("Cannot create ") + level_name);
}

}	 // namespace fragile::framework
