#include "worldgen/perf_observability.h"

WorldgenPerfTracker::Scope::Scope(WorldgenPerfTracker& tracker, std::string stage) :
	tracker(tracker),
	stage(std::move(stage)),
	started_at(std::chrono::steady_clock::now()) {
}

WorldgenPerfTracker::Scope::~Scope() {
	const auto ended_at = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration<double, std::milli>(ended_at - started_at).count();
	tracker.addSample(stage, elapsed);
}

void WorldgenPerfTracker::addSample(const std::string& stage, double milliseconds) {
	timeline.push_back({
		.stage = stage,
		.milliseconds = milliseconds
	});
}

double WorldgenPerfTracker::totalMs() const {
	double total = 0.0;
	for (const auto& sample : timeline) {
		total += sample.milliseconds;
	}
	return total;
}

const std::vector<WorldgenPerformanceSample>& WorldgenPerfTracker::samples() const {
	return timeline;
}
