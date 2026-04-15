#include "editor/rules/map_rule_engine.h"

#include "map/map.h"
#include "map/tile.h"
#include "game/item.h"

#include <algorithm>

namespace {
bool tileMatches(const Tile* tile, const MapRuleFilter& filter) {
	if (!tile) {
		return false;
	}
	if (filter.floor.has_value() && tile->getPosition().z != *filter.floor) {
		return false;
	}
	if (filter.require_spawn && !tile->spawn) {
		return false;
	}
	if (filter.require_house_tile && !tile->isHouseTile()) {
		return false;
	}
	if (!filter.item_id.has_value() && !filter.action_id.has_value()) {
		return true;
	}
	return std::ranges::any_of(tile->items, [&](const auto& item) {
		if (!item) {
			return false;
		}
		if (filter.item_id.has_value() && item->getID() != *filter.item_id) {
			return false;
		}
		if (filter.action_id.has_value() && item->getActionID() != *filter.action_id) {
			return false;
		}
		return true;
	});
}
}

std::vector<Position> MapRuleEngine::selectByFilter(Map& map, const MapRuleFilter& filter) {
	std::vector<Position> result;
	result.reserve(static_cast<size_t>(map.getTileCount() / 16));
	std::ranges::for_each(map.tiles(), [&](auto& location) {
		Tile* tile = location.get();
		if (tileMatches(tile, filter)) {
			result.push_back(tile->getPosition());
		}
	});
	return result;
}

size_t MapRuleEngine::smartReplace(Map& map, uint16_t from_id, uint16_t to_id, const MapRuleFilter& filter) {
	size_t replacements = 0;
	std::ranges::for_each(map.tiles(), [&](auto& location) {
		Tile* tile = location.get();
		if (!tile || !tileMatches(tile, filter)) {
			return;
		}
		for (auto& item : tile->items) {
			if (!item || item->getID() != from_id) {
				continue;
			}
			item->setID(to_id);
			++replacements;
		}
	});
	if (replacements > 0) {
		map.doChange();
	}
	return replacements;
}
