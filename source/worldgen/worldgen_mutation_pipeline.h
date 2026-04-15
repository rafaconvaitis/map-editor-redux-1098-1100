#ifndef RME_WORLDGEN_MUTATION_PIPELINE_H_
#define RME_WORLDGEN_MUTATION_PIPELINE_H_

#include "worldgen/worldgen_types.h"
#include "worldgen/perf_observability.h"

#include <functional>
#include <vector>

class Map;
struct WorldgenVariationProfile;

class WorldgenMutationPipeline {
public:
	using PostHook = std::function<void(Map&, const WorldgenRequest&)>;

	void addPostHook(PostHook hook);
	void execute(Map& map, const WorldgenRequest& request, WorldgenPerfTracker& perf_tracker) const;

private:
	void applyCity(Map& map, const WorldgenRequest& request, const WorldgenVariationProfile& variation) const;
	void applyDungeon(Map& map, const WorldgenRequest& request, const WorldgenVariationProfile& variation) const;
	void applyHuntArea(Map& map, const WorldgenRequest& request, const WorldgenVariationProfile& variation) const;
	void setGroundIfValid(Map& map, int x, int y, int z, uint16_t id) const;
	static uint64_t hash2D(int x, int y, uint64_t seed);

	std::vector<PostHook> post_hooks;
};

#endif
