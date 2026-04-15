#include "worldgen/semantic_diff_engine.h"

#include "map/map.h"
#include "map/tile.h"

#include <format>

WorldgenSnapshot SemanticDiffEngine::snapshot(Map& map) const {
	WorldgenSnapshot shot;
	shot.tile_count = map.getTileCount();
	shot.spawn_count = static_cast<uint64_t>(std::distance(map.spawns.begin(), map.spawns.end()));
	shot.waypoint_count = static_cast<uint64_t>(map.waypoints.size());
	shot.house_count = static_cast<uint64_t>(std::distance(map.houses.begin(), map.houses.end()));

	for (const auto& tile_location : map.tiles()) {
		const Tile* tile = tile_location.get();
		if (!tile) {
			continue;
		}

		if (tile->isSelected()) {
			++shot.selected_tiles;
		}
		if (tile->isBlocking()) {
			++shot.blocking_tiles;
		}
		if (tile->isModified()) {
			++shot.modified_tiles;
		}
	}

	return shot;
}

SemanticDiffReport SemanticDiffEngine::compare(const std::optional<WorldgenSnapshot>& previous, const WorldgenSnapshot& current) const {
	SemanticDiffReport report;

	if (!previous.has_value()) {
		report.messages.emplace_back("Semantic diff baseline created (first worldgen run in this session).");
		return report;
	}

	report.has_previous_snapshot = true;
	const WorldgenSnapshot& prev = previous.value();

	report.messages.push_back(std::format("Tiles: {} -> {}", prev.tile_count, current.tile_count));
	report.messages.push_back(std::format("Blocking tiles: {} -> {}", prev.blocking_tiles, current.blocking_tiles));
	report.messages.push_back(std::format("Modified tiles: {} -> {}", prev.modified_tiles, current.modified_tiles));
	report.messages.push_back(std::format("Spawns: {} -> {}", prev.spawn_count, current.spawn_count));
	report.messages.push_back(std::format("Waypoints: {} -> {}", prev.waypoint_count, current.waypoint_count));
	report.messages.push_back(std::format("Houses: {} -> {}", prev.house_count, current.house_count));

	return report;
}
