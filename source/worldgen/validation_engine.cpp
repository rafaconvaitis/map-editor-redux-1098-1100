#include "worldgen/validation_engine.h"

#include "map/map.h"
#include "map/tile.h"

#include <algorithm>
#include <format>

WorldgenValidationReport WorldgenValidationEngine::validate(const Map& map, const WorldgenRegion& region) const {
	WorldgenValidationReport report;

	uint64_t total_slots = 0;
	uint64_t occupied_tiles = 0;
	uint64_t blocking_tiles = 0;

	for (int y = region.y1; y <= region.y2; ++y) {
		for (int x = region.x1; x <= region.x2; ++x) {
			++total_slots;
			const Tile* tile = map.getTile(x, y, region.z);
			if (!tile) {
				continue;
			}

			++occupied_tiles;
			if (tile->isBlocking()) {
				++blocking_tiles;
			}
		}
	}

	if (total_slots == 0) {
		report.messages.emplace_back("Validation skipped: region is empty.");
		return report;
	}

	report.tile_coverage_ratio = static_cast<double>(occupied_tiles) / static_cast<double>(total_slots);
	report.blocking_ratio = occupied_tiles == 0 ? 0.0 : static_cast<double>(blocking_tiles) / static_cast<double>(occupied_tiles);
	report.has_dense_blocking = report.blocking_ratio > 0.70;
	report.sparse_content = report.tile_coverage_ratio < 0.40;

	report.messages.push_back(std::format("Tile coverage: {:.1f}%", report.tile_coverage_ratio * 100.0));
	report.messages.push_back(std::format("Blocking density: {:.1f}%", report.blocking_ratio * 100.0));

	if (report.sparse_content) {
		report.messages.emplace_back("Warning: generated area is sparse; consider raising terrain density.");
	} else {
		report.messages.emplace_back("Coverage looks healthy for an initial generation pass.");
	}

	if (report.has_dense_blocking) {
		report.messages.emplace_back("Warning: blocking density is high; pathing can become frustrating.");
	} else {
		report.messages.emplace_back("Blocking density is within a mapper-friendly range.");
	}

	return report;
}
