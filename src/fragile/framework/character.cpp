
#include "fragile/framework/character.h"

#include "fragile/framework/input_component.h"
#include "fragile/utils/editor.h"
#include "fragile/utils/register_helper.h"

#include <algorithm>
#include <godot_cpp/classes/scene_tree.hpp>

namespace fragile::framework {

void FCharacter::_bind_methods() {
	ADD_METHOD(FCharacter, get_input_component);
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

FInputComponent* FCharacter::get_input_component() const {
	// FPlayerController parents its component directly under the possessed pawn.
	for (const auto& node : get_children()) {
		if (auto* ic = cast_to<FInputComponent>(node); ic != nullptr) {
			return ic;
		}
	}

	return nullptr;
}

}	 // namespace fragile::framework
