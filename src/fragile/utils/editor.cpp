#include "editor.h"

#include <godot_cpp/classes/engine.hpp>

namespace fragile::utils {

bool is_in_editor() {
	const auto* engine = godot::Engine::get_singleton();
	return engine != nullptr && engine->is_editor_hint();
}

}	 // namespace fragile::utils
