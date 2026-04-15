#ifndef RME_NAVIGATION_HISTORY_H_
#define RME_NAVIGATION_HISTORY_H_

#include "map/position.h"

#include <string>
#include <vector>

struct NavigationBookmark {
	std::string map_name;
	std::string label;
	Position position;
};

class NavigationHistory {
public:
	static NavigationHistory& instance();

	void addPosition(const std::string& map_name, const Position& position);
	void addBookmark(const std::string& map_name, const std::string& label, const Position& position);
	const std::vector<Position>& getRecentPositions() const {
		return recent_positions_;
	}
	const std::vector<NavigationBookmark>& getBookmarks() const {
		return bookmarks_;
	}

	void load();
	void save() const;

private:
	std::vector<Position> recent_positions_;
	std::vector<NavigationBookmark> bookmarks_;
};

#endif
