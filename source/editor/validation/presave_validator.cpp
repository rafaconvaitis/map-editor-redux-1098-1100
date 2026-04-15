#include "editor/validation/presave_validator.h"

#include "map/map.h"
#include "map/tile.h"
#include "game/item.h"

#include <algorithm>
#include <format>

namespace {
void pushIssue(PreSaveValidationReport& report, PreSaveIssueSeverity severity, std::string code, std::string message, int count) {
	if (count <= 0) {
		return;
	}

	if (severity == PreSaveIssueSeverity::Error) {
		report.error_count += 1;
	} else if (severity == PreSaveIssueSeverity::Warning) {
		report.warning_count += 1;
	}

	report.issues.push_back({
		.severity = severity,
		.code = std::move(code),
		.message = std::move(message),
		.count = count,
	});
}
}

std::vector<std::string> PreSaveValidationReport::toLines() const {
	std::vector<std::string> rows;
	rows.push_back(std::format("Errors: {} | Warnings: {}", error_count, warning_count));
	for (const auto& issue : issues) {
		const char* sev = issue.severity == PreSaveIssueSeverity::Error ? "ERROR" :
			(issue.severity == PreSaveIssueSeverity::Warning ? "WARN" : "INFO");
		rows.push_back(std::format("[{}] {} ({})", sev, issue.message, issue.count));
	}
	return rows;
}

PreSaveValidationReport PreSaveValidator::validate(Map& map) {
	PreSaveValidationReport report;
	int invalid_zone_tiles = 0;
	int houses_without_town = 0;
	int stale_spawn_refs = 0;
	int missing_spawn_refs = 0;
	int broken_waypoints = 0;
	int inconsistent_doors = 0;

	for (const auto& [house_id, house_ptr] : map.houses) {
		if (!house_ptr) {
			continue;
		}
		if (house_ptr->townid == 0 || map.towns.find(house_ptr->townid) == map.towns.end()) {
			++houses_without_town;
		}
	}

	for (const Position& pos : map.spawns) {
		Tile* tile = map.getTile(pos);
		if (!tile || !tile->spawn) {
			++stale_spawn_refs;
		}
	}

	std::ranges::for_each(map.tiles(), [&](auto& location) {
		Tile* tile = location.get();
		if (!tile) {
			return;
		}

		if (tile->hasInvalidZones()) {
			++invalid_zone_tiles;
		}
		if (tile->spawn) {
			Position pos = tile->getPosition();
			if (map.spawns.find(pos) == map.spawns.end()) {
				++missing_spawn_refs;
			}
		}

		const bool has_door_item = std::ranges::any_of(tile->items, [](const auto& item) {
			return item && item->isDoor();
		});
		if (has_door_item && !tile->isHouseTile()) {
			++inconsistent_doors;
		}
	});

	for (const auto& [name, wp] : map.waypoints.waypoints) {
		if (!wp || !wp->pos.isValid() || map.getTile(wp->pos) == nullptr) {
			++broken_waypoints;
		}
	}

	pushIssue(report, PreSaveIssueSeverity::Error, "invalid_zones", "Invalid zone tiles", invalid_zone_tiles);
	pushIssue(report, PreSaveIssueSeverity::Warning, "house_town_mismatch", "Houses without valid town", houses_without_town);
	pushIssue(report, PreSaveIssueSeverity::Warning, "stale_spawns", "Stale spawn references", stale_spawn_refs);
	pushIssue(report, PreSaveIssueSeverity::Warning, "missing_spawns", "Missing spawn references", missing_spawn_refs);
	pushIssue(report, PreSaveIssueSeverity::Warning, "broken_waypoints", "Broken waypoints", broken_waypoints);
	pushIssue(report, PreSaveIssueSeverity::Warning, "inconsistent_doors", "Doors outside house tiles", inconsistent_doors);
	return report;
}

PreSaveValidationReport PreSaveValidator::autoFix(Map& map) {
	map.cleanInvalidTiles(false);
	map.cleanInvalidZones(false);

	for (auto it = map.waypoints.waypoints.begin(); it != map.waypoints.waypoints.end();) {
		if (!it->second || !it->second->pos.isValid() || map.getTile(it->second->pos) == nullptr) {
			it = map.waypoints.waypoints.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = map.spawns.begin(); it != map.spawns.end();) {
		Tile* tile = map.getTile(*it);
		if (!tile || !tile->spawn) {
			it = map.spawns.erase(it);
		} else {
			++it;
		}
	}

	std::ranges::for_each(map.tiles(), [&](auto& location) {
		Tile* tile = location.get();
		if (!tile || !tile->spawn) {
			return;
		}
		Position pos = tile->getPosition();
		if (map.spawns.find(pos) == map.spawns.end()) {
			map.spawns.addSpawn(tile);
		}
	});

	for (auto& [house_id, house_ptr] : map.houses) {
		if (!house_ptr) {
			continue;
		}
		if (house_ptr->townid != 0 && map.towns.find(house_ptr->townid) == map.towns.end()) {
			house_ptr->townid = 0;
		}
	}

	map.doChange();
	return validate(map);
}
