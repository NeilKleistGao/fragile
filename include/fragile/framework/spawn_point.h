#ifndef __FRAGILE_SPAWN_POINT_H__
#define __FRAGILE_SPAWN_POINT_H__

#include "actor.h"

namespace fragile::framework {

class FSpawnPoint : public FActor {
	GDCLASS(FSpawnPoint, FActor)

protected:
	static void _bind_methods();

public:
	void _draw() override;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_SPAWN_POINT_H__
