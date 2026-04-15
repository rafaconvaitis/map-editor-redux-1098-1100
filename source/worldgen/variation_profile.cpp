#include "worldgen/variation_profile.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <string_view>

namespace {
std::string toLowerCopy(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

bool containsAny(std::string_view text, std::initializer_list<const char*> keywords) {
	for (const char* keyword : keywords) {
		if (text.find(keyword) != std::string_view::npos) {
			return true;
		}
	}
	return false;
}

uint64_t hashMix(uint64_t value) {
	value ^= value >> 33;
	value *= 0xff51afd7ed558ccdULL;
	value ^= value >> 33;
	value *= 0xc4ceb9fe1a85ec53ULL;
	value ^= value >> 33;
	return value;
}

uint64_t hashText(std::string_view text) {
	uint64_t value = 1469598103934665603ULL;
	for (unsigned char c : text) {
		value ^= c;
		value *= 1099511628211ULL;
	}
	return hashMix(value);
}
}

WorldgenVariationProfile WorldgenVariationProfileBuilder::derive(const WorldgenRequest& request) {
	WorldgenVariationProfile profile;
	// 10 x 10 x 10 x 5 x 5 = 25,000 structural variations.
	static constexpr uint64_t kBiomeVariants = 10;
	static constexpr uint64_t kLayoutVariants = 10;
	static constexpr uint64_t kPathVariants = 10;
	static constexpr uint64_t kDetailVariants = 5;
	static constexpr uint64_t kHazardVariants = 5;
	static constexpr uint64_t kVariationSpace = kBiomeVariants * kLayoutVariants * kPathVariants * kDetailVariants * kHazardVariants;
	profile.variation_space = kVariationSpace;

	const std::string prompt_lower = toLowerCopy(request.prompt);
	const uint64_t prompt_hash = hashText(prompt_lower);
	const uint64_t size_hash = hashMix(static_cast<uint64_t>(request.region.width()) * 1315423911ULL + static_cast<uint64_t>(request.region.height()) * 2654435761ULL);
	uint64_t mixed = hashMix(request.seed ^ prompt_hash ^ size_hash);

	profile.biome_axis = static_cast<int>(mixed % kBiomeVariants); mixed /= kBiomeVariants;
	profile.layout_axis = static_cast<int>(mixed % kLayoutVariants); mixed /= kLayoutVariants;
	profile.path_axis = static_cast<int>(mixed % kPathVariants); mixed /= kPathVariants;
	profile.detail_axis = static_cast<int>(mixed % kDetailVariants); mixed /= kDetailVariants;
	profile.hazard_axis = static_cast<int>(mixed % kHazardVariants);

	// Prompt bias so same size/seed can still adapt to theme language.
	if (containsAny(prompt_lower, { "city", "urban", "cidade", "village", "town" })) {
		profile.layout_axis = std::max(profile.layout_axis, 6);
	}
	if (containsAny(prompt_lower, { "cave", "dungeon", "caverna", "masmorra" })) {
		profile.biome_axis = (profile.biome_axis + 3) % static_cast<int>(kBiomeVariants);
	}
	if (containsAny(prompt_lower, { "detail", "detalh", "rich", "dense" })) {
		profile.detail_axis = std::min(4, profile.detail_axis + 2);
		profile.prefer_microdetail = true;
	}
	if (containsAny(prompt_lower, { "lava", "swamp", "water", "river", "rio", "pantano" })) {
		profile.hazard_axis = std::min(4, profile.hazard_axis + 1);
	}

	const int area = std::max(1, request.region.width() * request.region.height());
	const int area_scale = std::clamp(area / 1200, 0, 4);
	profile.scaled_road_spacing = std::clamp(request.preset.road_spacing + (profile.path_axis % 4) - 1 + area_scale / 2, 3, 14);
	profile.scaled_detail_density = std::clamp(8 + profile.detail_axis * 4 + area_scale * 2, 8, 36);
	profile.scaled_trail_count = std::clamp(1 + area_scale + profile.path_axis / 4, 1, 9);
	profile.scaled_liquid_passes = std::clamp(1 + profile.hazard_axis / 2, 1, 4);

	profile.variation_index = (((static_cast<uint64_t>(profile.biome_axis) * kLayoutVariants + static_cast<uint64_t>(profile.layout_axis)) * kPathVariants +
		static_cast<uint64_t>(profile.path_axis)) * kDetailVariants + static_cast<uint64_t>(profile.detail_axis)) * kHazardVariants +
		static_cast<uint64_t>(profile.hazard_axis);

	profile.signature = std::format(
		"V{:05}-B{}L{}P{}D{}H{}",
		profile.variation_index,
		profile.biome_axis,
		profile.layout_axis,
		profile.path_axis,
		profile.detail_axis,
		profile.hazard_axis
	);
	return profile;
}
