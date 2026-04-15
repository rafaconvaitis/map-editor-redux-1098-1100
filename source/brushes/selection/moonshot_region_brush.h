#ifndef RME_MOONSHOT_REGION_BRUSH_H_
#define RME_MOONSHOT_REGION_BRUSH_H_

#include "brushes/brush.h"

class MoonshotRegionBrush : public Brush {
public:
	MoonshotRegionBrush() = default;
	~MoonshotRegionBrush() override = default;

	bool canDraw(BaseMap* map, const Position& position) const override;
	void draw(BaseMap* map, Tile* tile, void* parameter = nullptr) override;
	void undraw(BaseMap* map, Tile* tile) override;

	std::string getName() const override;
	int getLookID() const override;
	bool canSmear() const override {
		return false;
	}
	bool oneSizeFitsAll() const override {
		return true;
	}
};

#endif
