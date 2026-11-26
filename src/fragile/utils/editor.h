
#ifndef __FRAGILE_EDITOR_H__
#define __FRAGILE_EDITOR_H__

#define FRAGILE_RUNTIME_API                  \
	if (fragile::utils::is_in_editor()) return

namespace fragile::utils {
bool is_in_editor();
}	 // namespace fragile::utils

#endif	// __FRAGILE_EDITOR_H__
