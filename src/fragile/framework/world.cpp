#include "world.h"

#include "fragile/utils/type_check.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>

namespace fragile::framework {

void FWorld::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("open_level", "level_name"), &FWorld::open_level);

	godot::ClassDB::bind_method(godot::D_METHOD("set_game_mode", "game_mode"), &FWorld::set_game_mode);
	godot::ClassDB::bind_method(godot::D_METHOD("get_game_mode"), &FWorld::get_game_mode);

	godot::ClassDB::bind_method(godot::D_METHOD("set_game_mode_prefab", "prefab"), &FWorld::set_game_mode_prefab);
	godot::ClassDB::bind_method(godot::D_METHOD("get_game_mode_prefab"), &FWorld::get_game_mode_prefab);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_mode_prefab", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_game_mode_prefab", "get_game_mode_prefab");
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

void FWorld::set_game_mode(FGameMode* game_mode) {
	if (this->game_mode == game_mode) {
		return;
	}

	// Assertion: the world adopts the game mode as its own child, so the
	// caller must hand over one that has no parent yet. A game mode is
	// never stolen from an existing parent.
	ERR_FAIL_COND_MSG(game_mode != nullptr && game_mode->get_parent() != nullptr, "Cannot set game mode: the game mode already has a parent.");

	if (this->game_mode != nullptr) {
		FGameMode* old_game_mode = this->game_mode;
		this->game_mode = nullptr;
		old_game_mode->set_world(nullptr);
		if (old_game_mode->get_parent() == this) {
			remove_child(old_game_mode);
		}
		old_game_mode->queue_free();
	}

	this->game_mode = game_mode;
	if (this->game_mode != nullptr) {
		this->game_mode->set_world(this);
		add_child(this->game_mode);
	}
}

FGameMode* FWorld::get_game_mode() const {
	return this->game_mode;
}

void FWorld::set_game_mode_prefab(godot::PackedScene* prefab) {
	if (prefab == nullptr) {
		game_mode_prefab = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, godot::StringName{ "FGameMode" })) {
		game_mode_prefab = prefab;
		return;
	}

	godot::print_error("cannot set game_mode_prefab: the scene root does not derive from FGameMode.");
}

godot::PackedScene* FWorld::get_game_mode_prefab() const {
	return game_mode_prefab;
}

void FWorld::_ready() {
	Node2D::_ready();

	if (game_mode == nullptr && game_mode_prefab != nullptr) {
		if (auto* mode = cast_to<FGameMode>(game_mode_prefab->instantiate()); mode != nullptr) {
			set_game_mode(mode);	// links it and adds it as a child node
		}
	}
}

void FWorld::_notification(int p_what) {
	if (p_what == godot::Object::NOTIFICATION_PREDELETE) {
		set_game_mode(nullptr);	 // the world owns its game mode: freeing the world frees it too
	}
}

}	 // namespace fragile::framework
