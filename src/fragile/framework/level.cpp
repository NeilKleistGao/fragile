#include "level.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>

namespace fragile::framework {

void FLevel::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("serialize", "data"), &FLevel::serialize);
	godot::ClassDB::bind_method(godot::D_METHOD("deserialize", "data"), &FLevel::deserialize);
	godot::ClassDB::bind_method(godot::D_METHOD("spawn_actor", "actor_type_name", "actor_name"), &FLevel::spawn_actor);
}

godot::Error FLevel::serialize(godot::Dictionary data) const {
	if (data.has(s_serialization_actors_name)) {
	serialize_fail:
		godot::print_error(godot::String("Cannot serialize level: ") + get_name());
		return godot::Error::ERR_INVALID_DATA;
	}

	auto actor_json = godot::Dictionary();
	for (const auto& v : actors) {
		const auto* actor = cast_to<FActor>(v.get_validated_object());
		if (actor == nullptr || actor_json.has(actor->get_name())) {
			goto serialize_fail;	// TODO: more error information?
		}

		auto sub_data = godot::Dictionary();
		sub_data[s_serialization_type_name] = actor->get_class();	 // TODO: should we let FActor do it?
		if (actor->serialize(sub_data) != godot::Error::OK) {
			goto serialize_fail;
		}

		actor_json[actor->get_name()] = sub_data;
	}

	data[s_serialization_actors_name] = actor_json;
	return godot::Error::OK;
}

godot::Error FLevel::deserialize(const godot::Dictionary& data) {
	if (!data.has(s_serialization_actors_name) || data[s_serialization_actors_name].get_type() != godot::Variant::DICTIONARY) {
	deserialize_fail:
		godot::print_error(godot::String("Cannot deserialize level: ") + get_name());
		return godot::Error::ERR_INVALID_DATA;
	}

	godot::Dictionary actor_json = data[s_serialization_actors_name];
	for (const auto& key : actor_json.keys()) {
		const auto key_type = key.get_type();
		if (key_type != godot::Variant::STRING && key_type != godot::Variant::STRING_NAME) {
			goto deserialize_fail;	// TODO: more error information?
		}

		const godot::StringName name = key;
		const auto& vd = actor_json[key];
		if (vd.get_type() != godot::Variant::DICTIONARY) {
			goto deserialize_fail;	// TODO: more error information?
		}

		const godot::Dictionary d = vd;
		if (!d.has(s_serialization_type_name)) {
			goto deserialize_fail;	// TODO: more error information?
		}

		if (const auto type = d[s_serialization_type_name].get_type(); type != godot::Variant::STRING && type != godot::Variant::STRING_NAME) {
			goto deserialize_fail;	// TODO: more error information?
		}

		if (auto* actor = spawn_actor(d[s_serialization_type_name], name); actor == nullptr || actor->deserialize(d) != godot::Error::OK) {
			goto deserialize_fail;	// TODO: more error information?
		}
	}

	return godot::Error::OK;
}

void FLevel::add_actor(FActor* actor) {
	for (const auto& v : actors) {
		if (const auto* act = cast_to<FActor>(v.get_validated_object()); act != nullptr && act->get_name() == actor->get_name()) {
			godot::print_error(godot::String("Duplicate actor name: ") + actor->get_name());
			return;
		}
	}

	actors.append(actor);
	add_child(actor);
}

FActor* FLevel::spawn_actor(const godot::StringName& actor_type_name, const godot::StringName& actor_name) {
	if (godot::ClassDB::class_exists(actor_type_name) && godot::ClassDB::can_instantiate(actor_type_name)) {
		Object* object = godot::ClassDB::instantiate(actor_type_name);
		if (auto* actor = cast_to<FActor>(object); actor != nullptr) {
			actor->set_name(actor_name);
			add_actor(actor);
			return actor;
		}

		if (object != nullptr && cast_to<godot::RefCounted>(object) == nullptr) {
			godot::memdelete(object);
		}
	}

	godot::print_error(godot::String("Cannot create ") + actor_type_name);
	return nullptr;
}

}	 // namespace fragile::framework
