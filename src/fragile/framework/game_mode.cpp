
#include "game_mode.h"

#include "world.h"

namespace fragile::framework {

void FGameMode::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("get_world"), &FGameMode::get_world);
}

void FGameMode::_notification(int p_what) {
	if (p_what != godot::Object::NOTIFICATION_PREDELETE || world == nullptr) {
		return;
	}

	// This node is being deleted: only drop the links. Going through
	// FWorld::set_game_mode would try to free this node a second time.
	if (world->game_mode == this) {
		world->game_mode = nullptr;
	}
	world = nullptr;
}

void FGameMode::set_world(FWorld* world) {
	this->world = world;
}

FWorld* FGameMode::get_world() const {
	return world;
}

}	 // namespace fragile::framework
