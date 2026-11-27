
#ifndef __FRAGILE_EDITOR_H__
#define __FRAGILE_EDITOR_H__

// Ends a void function early while the scene is merely being edited, so
// gameplay setup (spawning levels, actors, players) never runs in the editor.
#define FRAGILE_RUNTIME_API                  \
	if (fragile::utils::is_in_editor()) return

namespace fragile::utils {

/**
 * @brief Returns whether the extension currently runs inside the editor.
 *
 * Wraps Engine::is_editor_hint(); used by FRAGILE_RUNTIME_API to skip runtime
 * setup while the scene is only being edited. False in a game started from
 * the editor.
 *
 * @return @c true when running in the editor, @c false in a running game.
 */
bool is_in_editor();

}	 // namespace fragile::utils

#endif	// __FRAGILE_EDITOR_H__
