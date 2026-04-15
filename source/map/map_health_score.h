#ifndef RME_MAP_HEALTH_SCORE_H_
#define RME_MAP_HEALTH_SCORE_H_

#include <string>
#include <vector>

class Map;

struct MapHealthScoreReport {
	int score = 0;
	double blocking_ratio = 0.0;
	double spawn_coverage = 0.0;
	double empty_hotspot_ratio = 0.0;
	int choke_points = 0;
	std::vector<std::string> lines;
};

class MapHealthScorer {
public:
	static MapHealthScoreReport evaluate(Map& map);
};

#endif
