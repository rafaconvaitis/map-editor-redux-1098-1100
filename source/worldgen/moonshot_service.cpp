#include "worldgen/moonshot_service.h"
#include "worldgen/variation_profile.h"

#include "map/map.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

MoonshotWorldgenService::MoonshotWorldgenService() {
	mutation_pipeline.addPostHook([](Map& map, const WorldgenRequest& request) {
		(void)request;
		map.cleanInvalidZones(false);
	});
}

WorldgenExecutionReport MoonshotWorldgenService::execute(Map& map, const WorldgenRequest& request) {
	WorldgenExecutionReport report;
	WorldgenRequest prepared_request = request;
	const WorldgenVariationProfile variation = WorldgenVariationProfileBuilder::derive(prepared_request);
	prepared_request.variation_index = variation.variation_index;
	prepared_request.variation_space = variation.variation_space;
	prepared_request.variation_signature = variation.signature;
	report.request = prepared_request;
	report.variation_index = variation.variation_index;
	report.variation_space = variation.variation_space;
	report.variation_signature = variation.signature;

	WorldgenPerfTracker perf;
	mutation_pipeline.execute(map, prepared_request, perf);

	report.validation = validation_engine.validate(map, prepared_request.region);
	report.snapshot_after = semantic_diff_engine.snapshot(map);
	report.semantic_diff = semantic_diff_engine.compare(previous_snapshot, report.snapshot_after);
	report.performance = perf.samples();

	appendTimeline(prepared_request, report.snapshot_after);
	report.timeline_tail = timelineTail(5);
	previous_snapshot = report.snapshot_after;

	return report;
}

std::string MoonshotWorldgenService::nowIsoLike() const {
	const auto now = std::chrono::system_clock::now();
	const std::time_t current_time = std::chrono::system_clock::to_time_t(now);
	std::tm local_tm {};
#ifdef _WIN32
	localtime_s(&local_tm, &current_time);
#else
	localtime_r(&current_time, &local_tm);
#endif

	std::ostringstream out;
	out << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
	return out.str();
}

void MoonshotWorldgenService::appendTimeline(const WorldgenRequest& request, const WorldgenSnapshot& snapshot) {
	timeline.push_back({
		.timestamp = nowIsoLike(),
		.prompt = request.prompt,
		.preset_name = request.preset.display_name,
		.region = request.region,
		.snapshot = snapshot
	});
}

std::vector<TimelineEntry> MoonshotWorldgenService::timelineTail(size_t max_items) const {
	if (timeline.size() <= max_items) {
		return timeline;
	}
	return std::vector<TimelineEntry>(timeline.end() - static_cast<std::ptrdiff_t>(max_items), timeline.end());
}
