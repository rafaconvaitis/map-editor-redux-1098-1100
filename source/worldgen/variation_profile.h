#ifndef RME_WORLDGEN_VARIATION_PROFILE_H_
#define RME_WORLDGEN_VARIATION_PROFILE_H_

#include "worldgen/worldgen_types.h"

#include <cstdint>
#include <string>

struct WorldgenVariationProfile {
	uint64_t variation_index = 0;
	uint64_t variation_space = 0;
	std::string signature;

	int biome_axis = 0;
	int layout_axis = 0;
	int path_axis = 0;
	int detail_axis = 0;
	int hazard_axis = 0;

	int scaled_road_spacing = 6;
	int scaled_detail_density = 12;
	int scaled_trail_count = 2;
	int scaled_liquid_passes = 1;
	bool prefer_microdetail = false;
};

class WorldgenVariationProfileBuilder {
public:
	static WorldgenVariationProfile derive(const WorldgenRequest& request);
};

#endif
