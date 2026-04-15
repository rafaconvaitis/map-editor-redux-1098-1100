#include "worldgen/worldgen_mutation_pipeline.h"
#include "worldgen/variation_profile.h"

#include "map/map.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/items.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <vector>

namespace {
struct WorldgenAssetIndex {
	std::vector<uint16_t> ground_nature_ids;
	std::vector<uint16_t> ground_rock_ids;
	std::vector<uint16_t> ground_urban_stone_ids;
	std::vector<uint16_t> ground_urban_wood_ids;
	std::vector<uint16_t> ground_swamp_ids;
	std::vector<uint16_t> ground_water_ids;
	std::vector<uint16_t> ground_lava_ids;

	std::vector<uint16_t> foliage_ids;
	std::vector<uint16_t> rock_ids;
	std::vector<uint16_t> remains_ids;
	std::vector<uint16_t> hazard_ids;
	std::vector<uint16_t> furniture_ids;
	std::vector<uint16_t> wall_stone_ids;
	std::vector<uint16_t> wall_wood_ids;
};

struct WorldgenTheme {
	bool dense_details = false;
	bool wants_undead = false;
	bool wants_spider = false;
	bool wants_cave = false;
	bool wants_lava = false;
	bool wants_water = false;
	bool wants_swamp = false;
	bool wants_urban = false;
	bool wants_stone = false;
	bool wants_wood = false;
};

std::string toLowerCopy(const std::string& value) {
	std::string lower = value;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return lower;
}

bool containsAnyKeyword(const std::string& haystack, std::initializer_list<const char*> keywords) {
	for (const char* keyword : keywords) {
		if (haystack.find(keyword) != std::string::npos) {
			return true;
		}
	}
	return false;
}

void pushIfNew(std::vector<uint16_t>& values, uint16_t id) {
	if (std::find(values.begin(), values.end(), id) == values.end()) {
		values.push_back(id);
	}
}

uint64_t localHash2D(int x, int y, uint64_t seed) {
	uint64_t value = seed;
	value ^= static_cast<uint64_t>(x) + 0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2);
	value ^= static_cast<uint64_t>(y) + 0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2);
	value ^= value >> 33;
	value *= 0xff51afd7ed558ccdULL;
	value ^= value >> 33;
	value *= 0xc4ceb9fe1a85ec53ULL;
	value ^= value >> 33;
	return value;
}

bool isDecorativeCandidate(const ItemType& type) {
	if (type.id == 0 || type.name.empty()) {
		return false;
	}
	if (type.isGroundTile() || type.isDoor() || type.isWall || type.isBorder || type.isTable || type.isCarpet) {
		return false;
	}
	return true;
}

WorldgenAssetIndex buildAssetIndexFromClientItems() {
	WorldgenAssetIndex palette;
	const int max_id = g_items.getMaxID();
	for (int id = 1; id <= max_id; ++id) {
		if (!g_items.typeExists(id)) {
			continue;
		}

		const ItemType type = g_items.getItemType(id);
		const std::string name = toLowerCopy(type.name);
		const uint16_t item_id = static_cast<uint16_t>(id);

		if (type.isGroundTile()) {
			if (containsAnyKeyword(name, { "lava", "magma", "fire", "fogo" })) {
				pushIfNew(palette.ground_lava_ids, item_id);
			} else if (containsAnyKeyword(name, { "swamp", "mud", "bog", "marsh", "pantano", "lodo", "lama" })) {
				pushIfNew(palette.ground_swamp_ids, item_id);
			} else if (containsAnyKeyword(name, { "water", "river", "sea", "ocean", "pool", "agua", "rio", "mar", "lago" })) {
				pushIfNew(palette.ground_water_ids, item_id);
			} else if (containsAnyKeyword(name, { "stone", "rock", "gravel", "cobble", "floor", "pedra", "rocha", "cascalho", "piso" })) {
				pushIfNew(palette.ground_rock_ids, item_id);
				pushIfNew(palette.ground_urban_stone_ids, item_id);
			} else if (containsAnyKeyword(name, { "wood", "plank", "timber", "board", "madeira", "tabua" })) {
				pushIfNew(palette.ground_urban_wood_ids, item_id);
			} else {
				pushIfNew(palette.ground_nature_ids, item_id);
			}
			continue;
		}

		if (type.isWall) {
			if (containsAnyKeyword(name, { "wood", "timber", "madeira" })) {
				pushIfNew(palette.wall_wood_ids, item_id);
			} else {
				pushIfNew(palette.wall_stone_ids, item_id);
			}
		}

		if (!isDecorativeCandidate(type)) {
			continue;
		}

		if (containsAnyKeyword(name, { "tree", "bush", "grass", "flower", "moss", "vine", "fern", "plant", "leaf", "arvore", "grama", "planta", "folha" })) {
			pushIfNew(palette.foliage_ids, item_id);
		} else if (containsAnyKeyword(name, { "stone", "rock", "boulder", "stalag", "pillar", "pedra", "rocha", "coluna" })) {
			pushIfNew(palette.rock_ids, item_id);
		} else if (containsAnyKeyword(name, { "bone", "skull", "corpse", "remains", "skeleton", "osso", "cranio", "esqueleto", "cadaver" })) {
			pushIfNew(palette.remains_ids, item_id);
		} else if (containsAnyKeyword(name, { "web", "cobweb", "lava", "fire", "poison", "spike", "teia", "veneno", "espinho" })) {
			pushIfNew(palette.hazard_ids, item_id);
		} else if (containsAnyKeyword(name, { "chair", "table", "bed", "cabinet", "shelf", "barrel", "crate", "fence", "cadeira", "mesa", "cama", "armario", "estante", "barril", "caixa", "cerca" })) {
			pushIfNew(palette.furniture_ids, item_id);
		}
	}
	return palette;
}

uint16_t pickDetailId(const std::vector<uint16_t>& ids, uint64_t noise) {
	if (ids.empty()) {
		return 0;
	}
	return ids[noise % ids.size()];
}

void placeDetailIfPossible(Map& map, int x, int y, int z, uint16_t id) {
	if (id == 0 || !g_items.typeExists(id)) {
		return;
	}

	Tile* tile = map.getTile(x, y, z);
	if (!tile || !tile->ground) {
		return;
	}
	if (tile->items.size() >= 4) {
		return;
	}

	tile->addItem(Item::Create(id));
}

bool promptWantsDenseDetails(const std::string& prompt_lower) {
	return containsAnyKeyword(prompt_lower, {
		"detailed", "detalhado", "detalhes", "rich", "dense", "complex", "complexo", "immersive", "imersivo"
	});
}

WorldgenTheme parseTheme(const std::string& prompt_lower) {
	WorldgenTheme theme;
	theme.dense_details = promptWantsDenseDetails(prompt_lower);
	theme.wants_undead = containsAnyKeyword(prompt_lower, { "undead", "necrom", "skeleton", "bone", "cemet", "crypt", "esqueleto", "morto-vivo", "cemiterio" });
	theme.wants_spider = containsAnyKeyword(prompt_lower, { "spider", "web", "teia", "aranha" });
	theme.wants_cave = containsAnyKeyword(prompt_lower, { "cave", "dungeon", "cavern", "mine", "masmorra", "caverna", "mina" });
	theme.wants_lava = containsAnyKeyword(prompt_lower, { "lava", "magma", "volcano", "vulcan", "vulcao", "inferno" });
	theme.wants_water = containsAnyKeyword(prompt_lower, { "river", "water", "lake", "waterfall", "rio", "agua", "cachoeira", "lago" });
	theme.wants_swamp = containsAnyKeyword(prompt_lower, { "swamp", "bog", "marsh", "pantano", "lodo", "lama" });
	theme.wants_urban = containsAnyKeyword(prompt_lower, { "city", "town", "urban", "village", "cidade", "urbana", "vilarejo", "rua", "street" });
	theme.wants_stone = containsAnyKeyword(prompt_lower, { "stone", "rock", "pedra", "rocha" });
	theme.wants_wood = containsAnyKeyword(prompt_lower, { "wood", "timber", "madeira" });
	return theme;
}

uint16_t pickGround(const std::vector<uint16_t>& preferred, uint16_t fallback, uint64_t noise = 0) {
	if (preferred.empty()) {
		return fallback;
	}
	return preferred[noise % preferred.size()];
}

void carveLiquidRibbon(Map& map, const WorldgenRegion& region, uint16_t liquid_ground_id, uint16_t shore_ground_id, uint64_t seed) {
	if (liquid_ground_id == 0 || !g_items.typeExists(liquid_ground_id)) {
		return;
	}

	int x = region.x1 + static_cast<int>((seed >> 7) % std::max(1, region.width()));
	int y = region.y1 + static_cast<int>((seed >> 19) % std::max(1, region.height()));
	const int steps = std::max(region.width(), region.height()) + region.width() / 2;
	for (int i = 0; i < steps; ++i) {
		const int width = (localHash2D(i, x + y, seed) % 100) < 25 ? 2 : 1;
		for (int dx = -width; dx <= width; ++dx) {
			for (int dy = -width; dy <= width; ++dy) {
				const int nx = std::clamp(x + dx, region.x1, region.x2);
				const int ny = std::clamp(y + dy, region.y1, region.y2);
				Tile* tile = map.getOrCreateTile(Position(nx, ny, region.z));
				if (!tile) {
					continue;
				}
				tile->ground = Item::Create(liquid_ground_id);
			}
		}

		if (shore_ground_id != 0 && g_items.typeExists(shore_ground_id)) {
			for (int nx = std::max(region.x1, x - 3); nx <= std::min(region.x2, x + 3); ++nx) {
				for (int ny = std::max(region.y1, y - 3); ny <= std::min(region.y2, y + 3); ++ny) {
					Tile* tile = map.getOrCreateTile(Position(nx, ny, region.z));
					if (!tile || !tile->ground || tile->ground->getID() == liquid_ground_id) {
						continue;
					}
					if ((localHash2D(nx, ny, seed ^ 0xabcdefULL) % 100) < 35) {
						tile->ground = Item::Create(shore_ground_id);
					}
				}
			}
		}

		switch (localHash2D(i, x, seed) % 4) {
			case 0: x += 1; break;
			case 1: x -= 1; break;
			case 2: y += 1; break;
			default: y -= 1; break;
		}
		x = std::clamp(x, region.x1, region.x2);
		y = std::clamp(y, region.y1, region.y2);
	}
}
}

void WorldgenMutationPipeline::addPostHook(PostHook hook) {
	post_hooks.push_back(std::move(hook));
}

void WorldgenMutationPipeline::execute(Map& map, const WorldgenRequest& request, WorldgenPerfTracker& perf_tracker) const {
	const WorldgenVariationProfile variation = WorldgenVariationProfileBuilder::derive(request);
	{
		WorldgenPerfTracker::Scope scope(perf_tracker, "MutationPass");
		switch (request.preset.kind) {
			case WorldgenPresetKind::City:
				applyCity(map, request, variation);
				break;
			case WorldgenPresetKind::Dungeon:
				applyDungeon(map, request, variation);
				break;
			case WorldgenPresetKind::HuntArea:
				applyHuntArea(map, request, variation);
				break;
		}
	}

	{
		WorldgenPerfTracker::Scope scope(perf_tracker, "PostHooks");
		for (const auto& hook : post_hooks) {
			hook(map, request);
		}
	}

	map.doChange();
}

void WorldgenMutationPipeline::applyCity(Map& map, const WorldgenRequest& request, const WorldgenVariationProfile& variation) const {
	const WorldgenRegion& r = request.region;
	const int spacing = std::max(3, variation.scaled_road_spacing);
	const WorldgenAssetIndex assets = buildAssetIndexFromClientItems();
	const std::string prompt_lower = toLowerCopy(request.prompt);
	const WorldgenTheme theme = parseTheme(prompt_lower);

	const uint16_t base_stone = pickGround(assets.ground_urban_stone_ids, request.preset.base_ground_id, request.seed);
	const uint16_t base_wood = pickGround(assets.ground_urban_wood_ids, base_stone, request.seed >> 1);
	const uint16_t road_ground = pickGround(assets.ground_urban_stone_ids, request.preset.road_ground_id, request.seed ^ 0x123456ULL);
	const uint16_t plaza_ground = pickGround(assets.ground_rock_ids, request.preset.accent_ground_id, request.seed ^ 0x654321ULL);
	const uint16_t wall_id = theme.wants_wood ? pickDetailId(assets.wall_wood_ids, request.seed) : pickDetailId(assets.wall_stone_ids, request.seed);

	for (int y = r.y1; y <= r.y2; ++y) {
		for (int x = r.x1; x <= r.x2; ++x) {
			const bool is_road_x = ((x - r.x1) % spacing) == 0;
			const bool is_road_y = ((y - r.y1) % spacing) == 0;
			const bool is_road = is_road_x || is_road_y;
			const bool wood_block = ((hash2D(x / 5, y / 5, request.seed) % 100) < 28) && !theme.wants_stone;
			const bool plaza_patch = (hash2D(x / 8, y / 8, request.seed ^ 0xbbbULL) % 100) < 10;
			uint16_t tile_id = wood_block ? base_wood : base_stone;
			if (is_road) {
				tile_id = road_ground;
			} else if (plaza_patch) {
				tile_id = plaza_ground;
			}
			setGroundIfValid(map, x, y, r.z, tile_id);

			if (wall_id != 0 && !is_road) {
				const bool district_border = ((x - r.x1) % spacing) == 1 || ((y - r.y1) % spacing) == 1;
				if (district_border && (hash2D(x, y, request.seed ^ 0x222ULL) % 100) < (6 + variation.hazard_axis)) {
					placeDetailIfPossible(map, x, y, r.z, wall_id);
				}
			}
			if (!assets.furniture_ids.empty() && !is_road && (hash2D(x, y, request.seed ^ 0x333ULL) % 100) < (2 + variation.detail_axis)) {
				placeDetailIfPossible(map, x, y, r.z, pickDetailId(assets.furniture_ids, hash2D(x, y, request.seed)));
			}
		}
	}
}

void WorldgenMutationPipeline::applyDungeon(Map& map, const WorldgenRequest& request, const WorldgenVariationProfile& variation) const {
	const WorldgenRegion& r = request.region;
	const int w = std::max(1, r.width());
	const int h = std::max(1, r.height());

	for (int y = r.y1; y <= r.y2; ++y) {
		for (int x = r.x1; x <= r.x2; ++x) {
			const int nx = x - r.x1;
			const int ny = y - r.y1;
			const bool border = nx == 0 || ny == 0 || nx == (w - 1) || ny == (h - 1);
			const bool corridor = (nx % std::max(4, variation.scaled_road_spacing)) == 0 || (ny % std::max(4, variation.scaled_road_spacing)) == 0;

			if (border || corridor) {
				setGroundIfValid(map, x, y, r.z, request.preset.accent_ground_id);
			} else {
				const uint64_t noise = hash2D(x, y, request.seed);
				const bool room_patch = (noise % 100) < (10 + variation.detail_axis * 2);
				setGroundIfValid(map, x, y, r.z, room_patch ? request.preset.base_ground_id : request.preset.accent_ground_id);
			}
		}
	}
}

void WorldgenMutationPipeline::applyHuntArea(Map& map, const WorldgenRequest& request, const WorldgenVariationProfile& variation) const {
	const WorldgenRegion& r = request.region;
	const double accent_ratio = std::clamp(request.preset.accent_ratio, 0.08, 0.85);
	const uint64_t threshold = static_cast<uint64_t>(accent_ratio * 1000.0);
	const std::string prompt_lower = toLowerCopy(request.prompt);
	const WorldgenTheme theme = parseTheme(prompt_lower);
	const bool dense_details = theme.dense_details;
	const WorldgenAssetIndex assets = buildAssetIndexFromClientItems();

	const uint16_t natural_ground = pickGround(assets.ground_nature_ids, request.preset.base_ground_id, request.seed);
	const uint16_t rocky_ground = pickGround(assets.ground_rock_ids, request.preset.accent_ground_id, request.seed ^ 0x1976d2ULL);
	const uint16_t swamp_ground = pickGround(assets.ground_swamp_ids, rocky_ground, request.seed ^ 0x0f0f0fULL);
	const uint16_t water_ground = pickGround(assets.ground_water_ids, request.preset.road_ground_id, request.seed ^ 0x00ff00ULL);
	const uint16_t lava_ground = pickGround(assets.ground_lava_ids, rocky_ground, request.seed ^ 0xff3300ULL);
	const uint16_t trail_ground = theme.wants_urban ? pickGround(assets.ground_urban_stone_ids, request.preset.road_ground_id, request.seed ^ 0x333333ULL) : rocky_ground;

	for (int y = r.y1; y <= r.y2; ++y) {
		for (int x = r.x1; x <= r.x2; ++x) {
			const uint64_t macro_noise = hash2D(x / 6, y / 6, request.seed ^ 0x1f3d5b79ULL) % 1000;
			const uint64_t micro_noise = hash2D(x, y, request.seed ^ 0x9e3779b97f4a7c15ULL) % 1000;
			const uint64_t blend = (macro_noise * 7 + micro_noise * 3) / 10;

			const bool accent = blend < threshold;
			const bool clearing = (hash2D(x / 11, y / 11, request.seed ^ 0x5deece66dULL) % 1000) < 90;
			uint16_t base_ground = theme.wants_cave ? rocky_ground : natural_ground;
			uint16_t accent_ground = rocky_ground;
			if (theme.wants_swamp) {
				accent_ground = swamp_ground;
			}
			if (theme.wants_lava) {
				accent_ground = lava_ground;
			}
			if (theme.wants_water && !theme.wants_lava) {
				accent_ground = water_ground;
			}
			uint16_t ground_id = accent ? accent_ground : base_ground;
			if (clearing) {
				ground_id = trail_ground;
			}
			setGroundIfValid(map, x, y, r.z, ground_id);
		}
	}

	// Carve deterministic trails so hunt areas are navigable and less "flat blob".
	const int trail_count = std::max(1, variation.scaled_trail_count + (dense_details ? 1 : 0));
	const int trail_steps = std::max(r.width(), r.height()) + std::min(r.width(), r.height()) / 2;
	for (int t = 0; t < trail_count; ++t) {
		const uint64_t trail_seed = request.seed + static_cast<uint64_t>(t) * 2654435761ULL;
		int x = r.x1 + static_cast<int>(trail_seed % std::max(1, r.width()));
		int y = r.y1 + static_cast<int>((trail_seed >> 16) % std::max(1, r.height()));
		for (int step = 0; step < trail_steps; ++step) {
			setGroundIfValid(map, x, y, r.z, trail_ground);
			if ((hash2D(x, y, trail_seed + step) % 100) < 28) {
				setGroundIfValid(map, std::clamp(x + 1, r.x1, r.x2), y, r.z, trail_ground);
			}

			const uint64_t turn = hash2D(step, t, trail_seed) % 4;
			switch (turn) {
				case 0: x += 1; break;
				case 1: x -= 1; break;
				case 2: y += 1; break;
				default: y -= 1; break;
			}
			x = std::clamp(x, r.x1, r.x2);
			y = std::clamp(y, r.y1, r.y2);
		}
	}

	if (theme.wants_lava) {
		for (int i = 0; i < variation.scaled_liquid_passes; ++i) {
			carveLiquidRibbon(map, r, lava_ground, rocky_ground, request.seed ^ (0xabc111ULL + i * 977ULL));
		}
	} else if (theme.wants_swamp) {
		for (int i = 0; i < variation.scaled_liquid_passes; ++i) {
			carveLiquidRibbon(map, r, swamp_ground, natural_ground, request.seed ^ (0xabc222ULL + i * 977ULL));
		}
	} else if (theme.wants_water) {
		for (int i = 0; i < variation.scaled_liquid_passes; ++i) {
			carveLiquidRibbon(map, r, water_ground, natural_ground, request.seed ^ (0xabc333ULL + i * 977ULL));
		}
	}

	// Use loaded DAT/SPR names to place details in thematic clusters.
	const int detail_chance = dense_details ? variation.scaled_detail_density : std::max(9, variation.scaled_detail_density - 8);
	const bool undead_theme = theme.wants_undead;
	const bool spider_theme = theme.wants_spider;

	for (int y = r.y1; y <= r.y2; ++y) {
		for (int x = r.x1; x <= r.x2; ++x) {
			const uint64_t noise = hash2D(x, y, request.seed ^ 0x94d049bb133111ebULL);
			if ((noise % 100) >= static_cast<uint64_t>(detail_chance)) {
				continue;
			}

			uint16_t detail_id = 0;
			if (undead_theme) {
				detail_id = pickDetailId(assets.remains_ids, noise);
				if (detail_id == 0) {
					detail_id = pickDetailId(assets.rock_ids, noise);
				}
			} else if (spider_theme) {
				detail_id = pickDetailId(assets.hazard_ids, noise);
				if (detail_id == 0) {
					detail_id = pickDetailId(assets.foliage_ids, noise);
				}
			} else if (theme.wants_urban) {
				detail_id = pickDetailId(assets.furniture_ids, noise);
				if (detail_id == 0) {
					detail_id = pickDetailId(theme.wants_wood ? assets.wall_wood_ids : assets.wall_stone_ids, noise);
				}
			} else {
				const uint64_t bucket = noise % 10;
				if (bucket < 6) {
					detail_id = pickDetailId(assets.foliage_ids, noise);
				} else if (bucket < 8) {
					detail_id = pickDetailId(assets.rock_ids, noise);
				} else {
					detail_id = pickDetailId(assets.remains_ids, noise);
				}
			}
			placeDetailIfPossible(map, x, y, r.z, detail_id);
		}
	}
}

void WorldgenMutationPipeline::setGroundIfValid(Map& map, int x, int y, int z, uint16_t id) const {
	if (!g_items.typeExists(id)) {
		return;
	}

	ItemType type = g_items.getItemType(id);
	if (!type.isGroundTile()) {
		return;
	}

	Tile* tile = map.getOrCreateTile(Position(x, y, z));
	if (!tile) {
		return;
	}

	tile->ground = Item::Create(id);
}

uint64_t WorldgenMutationPipeline::hash2D(int x, int y, uint64_t seed) {
	uint64_t value = seed;
	value ^= static_cast<uint64_t>(x) + 0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2);
	value ^= static_cast<uint64_t>(y) + 0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2);
	value ^= value >> 33;
	value *= 0xff51afd7ed558ccdULL;
	value ^= value >> 33;
	value *= 0xc4ceb9fe1a85ec53ULL;
	value ^= value >> 33;
	return value;
}
