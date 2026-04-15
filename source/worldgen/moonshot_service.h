#ifndef RME_WORLDGEN_MOONSHOT_SERVICE_H_
#define RME_WORLDGEN_MOONSHOT_SERVICE_H_

#include "worldgen/worldgen_types.h"
#include "worldgen/worldgen_mutation_pipeline.h"
#include "worldgen/validation_engine.h"
#include "worldgen/semantic_diff_engine.h"

#include <optional>
#include <vector>

class Map;

class MoonshotWorldgenService {
public:
	MoonshotWorldgenService();

	WorldgenExecutionReport execute(Map& map, const WorldgenRequest& request);

private:
	std::string nowIsoLike() const;
	void appendTimeline(const WorldgenRequest& request, const WorldgenSnapshot& snapshot);
	std::vector<TimelineEntry> timelineTail(size_t max_items) const;

	WorldgenMutationPipeline mutation_pipeline;
	WorldgenValidationEngine validation_engine;
	SemanticDiffEngine semantic_diff_engine;

	std::optional<WorldgenSnapshot> previous_snapshot;
	std::vector<TimelineEntry> timeline;
};

#endif
