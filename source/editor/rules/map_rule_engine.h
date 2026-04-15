#ifndef RME_MAP_RULE_ENGINE_H_
#define RME_MAP_RULE_ENGINE_H_

#include "map/position.h"

#include <optional>
#include <vector>

class Map;
class Tile;

struct MapRuleFilter {
	std::optional<uint16_t> item_id;
	std::optional<uint16_t> action_id;
	bool require_spawn = false;
	bool require_house_tile = false;
	std::optional<uint8_t> floor;
};

class MapRuleEngine {
public:
	static std::vector<Position> selectByFilter(Map& map, const MapRuleFilter& filter);
	static size_t smartReplace(Map& map, uint16_t from_id, uint16_t to_id, const MapRuleFilter& filter);
};

#endif
