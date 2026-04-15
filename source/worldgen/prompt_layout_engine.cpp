#include "worldgen/prompt_layout_engine.h"

#include "worldgen/worldgen_preset_registry.h"

#include <algorithm>
#include <array>

PromptDraft PromptLayoutEngine::buildDraft(const std::string& prompt) const {
	const std::string normalized = toLower(prompt);

	const auto containsAny = [&normalized](const std::array<const char*, 6>& words) {
		return std::any_of(words.begin(), words.end(), [&normalized](const char* word) {
			return normalized.find(word) != std::string::npos;
		});
	};

	const bool asks_for_detail = normalized.find("detail") != std::string::npos ||
		normalized.find("detalh") != std::string::npos ||
		normalized.find("rich") != std::string::npos ||
		normalized.find("dense") != std::string::npos;

	if (containsAny({ "city", "town", "village", "urban", "cidade", "vilarejo" })) {
		return {
			.preset = WorldgenPresetRegistry::find("city").value_or(WorldgenPresetRegistry::defaultPreset()),
			.summary = asks_for_detail ? "City layout detected (roads + districts + richer detailing)." : "City layout detected from prompt (roads + districts).",
			.confidence = 0.91
		};
	}

	if (containsAny({ "dungeon", "cave", "crypt", "ruin", "masmorra", "caverna" })) {
		return {
			.preset = WorldgenPresetRegistry::find("dungeon").value_or(WorldgenPresetRegistry::defaultPreset()),
			.summary = asks_for_detail ? "Dungeon layout detected (corridors + rooms + stronger ambient detail)." : "Dungeon layout detected from prompt (corridors + rooms).",
			.confidence = 0.89
		};
	}

	if (containsAny({ "hunt", "spawn", "wilderness", "forest", "caca", "hunting" })) {
		return {
			.preset = WorldgenPresetRegistry::find("hunt").value_or(WorldgenPresetRegistry::defaultPreset()),
			.summary = asks_for_detail ? "Hunt-area layout detected (organic terrain + denser detail pass)." : "Hunt-area layout detected from prompt (organic terrain mix).",
			.confidence = 0.87
		};
	}

	return {
		.preset = WorldgenPresetRegistry::defaultPreset(),
		.summary = "No strong keyword detected. Falling back to Hunt Area preset.",
		.confidence = 0.55
	};
}

std::string PromptLayoutEngine::toLower(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return text;
}
