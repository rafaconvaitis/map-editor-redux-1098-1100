#ifndef RME_WORLDGEN_SEMANTIC_DIFF_ENGINE_H_
#define RME_WORLDGEN_SEMANTIC_DIFF_ENGINE_H_

#include "worldgen/worldgen_types.h"

#include <optional>

class Map;

class SemanticDiffEngine {
public:
	WorldgenSnapshot snapshot(Map& map) const;
	SemanticDiffReport compare(const std::optional<WorldgenSnapshot>& previous, const WorldgenSnapshot& current) const;
};

#endif
