#include "fragile/framework/spawn_point.h"

#include "fragile/utils/editor.h"

namespace fragile::framework {

void FSpawnPoint::_bind_methods() {
}

void FSpawnPoint::_draw() {
	FActor::_draw();

	if (utils::is_in_editor()) {
		constexpr float line_ext{ 8.0f };
		constexpr Color color{ 1.0f, 1.0f, 1.0f, 1.0f };

		draw_line(Vector2{ -line_ext, -line_ext }, Vector2{ line_ext, line_ext }, color, 2);
		draw_line(Vector2{ line_ext, -line_ext }, Vector2{ -line_ext, line_ext }, color, 2);
	}
}

}	 // namespace fragile::framework
