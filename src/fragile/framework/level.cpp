#include "level.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>

namespace fragile::framework {

void FLevel::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("spawn_actor", "actor_type_name", "actor_name"), &FLevel::spawn_actor);
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
