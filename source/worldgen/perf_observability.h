#ifndef RME_WORLDGEN_PERF_OBSERVABILITY_H_
#define RME_WORLDGEN_PERF_OBSERVABILITY_H_

#include "worldgen/worldgen_types.h"

#include <chrono>
#include <string>
#include <vector>

class WorldgenPerfTracker {
public:
	class Scope {
	public:
		Scope(WorldgenPerfTracker& tracker, std::string stage);
		~Scope();

	private:
		WorldgenPerfTracker& tracker;
		std::string stage;
		std::chrono::steady_clock::time_point started_at;
	};

	void addSample(const std::string& stage, double milliseconds);
	double totalMs() const;
	const std::vector<WorldgenPerformanceSample>& samples() const;

private:
	std::vector<WorldgenPerformanceSample> timeline;
};

#endif
