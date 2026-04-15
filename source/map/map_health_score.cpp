#include "map/map_health_score.h"

#include "map/map.h"
#include "map/tile.h"

#include <algorithm>
#include <format>

MapHealthScoreReport MapHealthScorer::evaluate(Map& map) {
	MapHealthScoreReport report;
	uint64_t total_tiles = 0;
	uint64_t blocking_tiles = 0;
	uint64_t tiles_with_spawn = 0;
	uint64_t empty_tiles = 0;
	uint64_t choke_points = 0;

	std::ranges::for_each(map.tiles(), [&](auto& location) {
		Tile* tile = location.get();
		if (!tile) {
			return;
		}
		++total_tiles;
		if (tile->isBlocking()) {
			++blocking_tiles;
		}
		if (tile->spawn) {
			++tiles_with_spawn;
		}
		const bool has_ground = tile->ground != nullptr;
		const bool has_content = has_ground || !tile->items.empty() || tile->spawn || tile->creature;
		if (!has_content) {
			++empty_tiles;
		}
		if (tile->isBlocking() && tile->items.size() >= 2) {
			++choke_points;
		}
	});

	if (total_tiles > 0) {
		report.blocking_ratio = static_cast<double>(blocking_tiles) / static_cast<double>(total_tiles);
		report.spawn_coverage = static_cast<double>(tiles_with_spawn) / static_cast<double>(total_tiles);
		report.empty_hotspot_ratio = static_cast<double>(empty_tiles) / static_cast<double>(total_tiles);
		report.choke_points = static_cast<int>(choke_points);
	}

	int score = 100;
	score -= static_cast<int>(std::clamp(report.blocking_ratio, 0.0, 1.0) * 30.0);
	score -= static_cast<int>(std::clamp(report.empty_hotspot_ratio, 0.0, 1.0) * 20.0);
	score += static_cast<int>(std::clamp(report.spawn_coverage, 0.0, 0.20) * 150.0);
	score += std::min(10, report.choke_points / 500);
	report.score = std::clamp(score, 0, 100);

	report.lines.push_back(std::format("Map Health Score: {}", report.score));
	report.lines.push_back(std::format("Blocking density: {:.2f}%", report.blocking_ratio * 100.0));
	report.lines.push_back(std::format("Spawn coverage: {:.2f}%", report.spawn_coverage * 100.0));
	report.lines.push_back(std::format("Empty hotspots: {:.2f}%", report.empty_hotspot_ratio * 100.0));
	report.lines.push_back(std::format("Estimated choke points: {}", report.choke_points));
	return report;
}
