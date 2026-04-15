#ifndef RME_WORLDGEN_VALIDATION_ENGINE_H_
#define RME_WORLDGEN_VALIDATION_ENGINE_H_

#include "worldgen/worldgen_types.h"

class Map;

class WorldgenValidationEngine {
public:
	WorldgenValidationReport validate(const Map& map, const WorldgenRegion& region) const;
};

#endif
