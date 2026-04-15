#ifndef RME_WORLDGEN_PRESET_REGISTRY_H_
#define RME_WORLDGEN_PRESET_REGISTRY_H_

#include "worldgen/worldgen_types.h"

#include <optional>
#include <string_view>
#include <vector>

class WorldgenPresetRegistry {
public:
	static const std::vector<WorldgenPreset>& all();
	static std::optional<WorldgenPreset> find(std::string_view id);
	static WorldgenPreset defaultPreset();
};

#endif
