#include "app/main.h"

#include "brushes/selection/moonshot_region_brush.h"
#include "game/sprites.h"

std::string MoonshotRegionBrush::getName() const {
	return "Moonshot Region Select";
}

int MoonshotRegionBrush::getLookID() const {
	return EDITOR_SPRITE_SELECTION_GEM;
}

bool MoonshotRegionBrush::canDraw(BaseMap* map, const Position& position) const {
	return true;
}

void MoonshotRegionBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	// Selection is handled in DrawingController for this brush.
	(void)map;
	(void)tile;
	(void)parameter;
}

void MoonshotRegionBrush::undraw(BaseMap* map, Tile* tile) {
	(void)map;
	(void)tile;
}
