
#include "fragile/framework/character.h"

#include "fragile/utils/editor.h"
#include "godot_cpp/classes/scene_tree.hpp"

#include <algorithm>

namespace fragile::framework {

void FCharacter::_bind_methods() {
}

void FCharacter::_ready() {
	FPawn::_ready();

	if (const auto children = get_children(); std::all_of(children.begin(), children.end(), [](const Variant& node) {
				return cast_to<AnimatedSprite2D>(node) == nullptr;
			})) {
		const auto sprite = memnew(AnimatedSprite2D);
		sprite->set_name("Sprite");
		add_child(sprite);

		if (utils::is_in_editor()) {
			sprite->set_owner(get_tree()->get_edited_scene_root());
		}
	}
}

}	 // namespace fragile::framework
