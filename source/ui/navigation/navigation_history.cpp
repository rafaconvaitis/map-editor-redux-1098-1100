#include "ui/navigation/navigation_history.h"

#include "app/main.h"
#include "util/file_system.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace {
std::filesystem::path storagePath() {
	return std::filesystem::u8path(nstr(FileSystem::GetLocalDataDirectory())) / "navigation_history.txt";
}
}

NavigationHistory& NavigationHistory::instance() {
	static NavigationHistory manager;
	return manager;
}

void NavigationHistory::addPosition(const std::string& map_name, const Position& position) {
	(void)map_name;
	if (!position.isValid()) {
		return;
	}
	recent_positions_.push_back(position);
	if (recent_positions_.size() > 200) {
		recent_positions_.erase(recent_positions_.begin(), recent_positions_.begin() + static_cast<long>(recent_positions_.size() - 200));
	}
}

void NavigationHistory::addBookmark(const std::string& map_name, const std::string& label, const Position& position) {
	if (!position.isValid() || label.empty()) {
		return;
	}
	bookmarks_.push_back({ map_name, label, position });
	if (bookmarks_.size() > 200) {
		bookmarks_.erase(bookmarks_.begin(), bookmarks_.begin() + static_cast<long>(bookmarks_.size() - 200));
	}
}

void NavigationHistory::load() {
	recent_positions_.clear();
	bookmarks_.clear();

	std::ifstream stream(storagePath(), std::ios::in);
	if (!stream.is_open()) {
		return;
	}

	std::string kind;
	while (stream >> kind) {
		if (kind == "P") {
			Position pos;
			stream >> pos.x >> pos.y >> pos.z;
			if (pos.isValid()) {
				recent_positions_.push_back(pos);
			}
		} else if (kind == "B") {
			NavigationBookmark bookmark;
			stream >> bookmark.map_name >> bookmark.label >> bookmark.position.x >> bookmark.position.y >> bookmark.position.z;
			if (bookmark.position.isValid()) {
				bookmarks_.push_back(std::move(bookmark));
			}
		}
	}
}

void NavigationHistory::save() const {
	std::ofstream stream(storagePath(), std::ios::trunc | std::ios::out);
	if (!stream.is_open()) {
		return;
	}

	for (const Position& pos : recent_positions_) {
		stream << "P " << pos.x << " " << pos.y << " " << int(pos.z) << "\n";
	}
	for (const auto& bookmark : bookmarks_) {
		stream << "B " << bookmark.map_name << " " << bookmark.label << " "
		       << bookmark.position.x << " " << bookmark.position.y << " " << int(bookmark.position.z) << "\n";
	}
}
