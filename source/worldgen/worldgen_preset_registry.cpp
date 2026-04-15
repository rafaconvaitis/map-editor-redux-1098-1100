#include "worldgen/worldgen_preset_registry.h"

namespace {
const std::vector<WorldgenPreset> kPresets = {
	{
		.kind = WorldgenPresetKind::City,
		.id = "city",
		.display_name = "City",
		.base_ground_id = 4526,
		.accent_ground_id = 4514,
		.road_ground_id = 106,
		.road_spacing = 6,
		.accent_ratio = 0.12
	},
	{
		.kind = WorldgenPresetKind::Dungeon,
		.id = "dungeon",
		.display_name = "Dungeon",
		.base_ground_id = 919,
		.accent_ground_id = 4526,
		.road_ground_id = 919,
		.road_spacing = 5,
		.accent_ratio = 0.08
	},
	{
		.kind = WorldgenPresetKind::HuntArea,
		.id = "hunt",
		.display_name = "Hunt Area",
		.base_ground_id = 4526,
		.accent_ground_id = 4608,
		.road_ground_id = 4526,
		.road_spacing = 8,
		.accent_ratio = 0.26
	}
};
}

const std::vector<WorldgenPreset>& WorldgenPresetRegistry::all() {
	return kPresets;
}

std::optional<WorldgenPreset> WorldgenPresetRegistry::find(std::string_view id) {
	for (const auto& preset : kPresets) {
		if (preset.id == id) {
			return preset;
		}
	}
	return std::nullopt;
}

WorldgenPreset WorldgenPresetRegistry::defaultPreset() {
	return kPresets.back();
}
