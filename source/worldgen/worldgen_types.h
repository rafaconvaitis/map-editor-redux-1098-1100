#ifndef RME_WORLDGEN_TYPES_H_
#define RME_WORLDGEN_TYPES_H_

#include <cstdint>
#include <string>
#include <vector>

struct WorldgenRegion {
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	int z = 7;

	int width() const {
		return x2 - x1 + 1;
	}

	int height() const {
		return y2 - y1 + 1;
	}
};

enum class WorldgenPresetKind {
	City,
	Dungeon,
	HuntArea
};

struct WorldgenPreset {
	WorldgenPresetKind kind = WorldgenPresetKind::HuntArea;
	std::string id;
	std::string display_name;
	uint16_t base_ground_id = 4526;
	uint16_t accent_ground_id = 4608;
	uint16_t road_ground_id = 4526;
	int road_spacing = 6;
	double accent_ratio = 0.18;
};

struct WorldgenRequest {
	uint64_t seed = 0;
	WorldgenPreset preset;
	WorldgenRegion region;
	std::string prompt;
	bool partial_apply = false;
	uint64_t variation_index = 0;
	uint64_t variation_space = 0;
	std::string variation_signature;
};

struct PromptDraft {
	WorldgenPreset preset;
	std::string summary;
	double confidence = 0.5;
};

struct WorldgenPerformanceSample {
	std::string stage;
	double milliseconds = 0.0;
};

struct WorldgenSnapshot {
	uint64_t tile_count = 0;
	uint64_t spawn_count = 0;
	uint64_t waypoint_count = 0;
	uint64_t house_count = 0;
	uint64_t selected_tiles = 0;
	uint64_t blocking_tiles = 0;
	uint64_t modified_tiles = 0;
};

struct WorldgenValidationReport {
	double blocking_ratio = 0.0;
	double tile_coverage_ratio = 0.0;
	bool has_dense_blocking = false;
	bool sparse_content = false;
	std::vector<std::string> messages;
};

struct SemanticDiffReport {
	std::vector<std::string> messages;
	bool has_previous_snapshot = false;
};

struct TimelineEntry {
	std::string timestamp;
	std::string prompt;
	std::string preset_name;
	WorldgenRegion region;
	WorldgenSnapshot snapshot;
};

struct WorldgenExecutionReport {
	WorldgenRequest request;
	std::string variation_signature;
	uint64_t variation_index = 0;
	uint64_t variation_space = 0;
	WorldgenValidationReport validation;
	SemanticDiffReport semantic_diff;
	WorldgenSnapshot snapshot_after;
	std::vector<WorldgenPerformanceSample> performance;
	std::vector<TimelineEntry> timeline_tail;
};

#endif
