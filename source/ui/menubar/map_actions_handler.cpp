#include "ui/menubar/map_actions_handler.h"
#include "ui/gui.h"
#include "ui/dialog_util.h"
#include "ui/find_item_window.h"
#include "editor/editor.h"
#include "editor/operations/clean_operations.h"
#include "editor/operations/search_operations.h"
#include "map/map.h"
#include "editor/action_queue.h"
#include <format>
#include <vector>

namespace {
struct MapAuditReport {
	uint64_t total_tiles = 0;
	uint64_t invalid_zone_tiles = 0;
	uint64_t orphan_house_tiles = 0;
	uint64_t orphan_houses = 0;
	uint64_t stale_spawn_refs = 0;
	uint64_t missing_spawn_refs = 0;
	uint64_t orphan_waypoints = 0;
};

MapAuditReport auditMap(Map& map) {
	MapAuditReport report;
	report.total_tiles = map.getTileCount();

	for (auto& tile_location : map) {
		Tile* tile = tile_location.get();
		if (!tile) {
			continue;
		}

		if (tile->hasInvalidZones()) {
			++report.invalid_zone_tiles;
		}

		if (tile->isHouseTile() && map.houses.getHouse(tile->getHouseID()) == nullptr) {
			++report.orphan_house_tiles;
		}

		Position tile_position = tile->getPosition();
		if (tile->spawn && map.spawns.find(tile_position) == map.spawns.end()) {
			++report.missing_spawn_refs;
		}
	}

	for (const auto& position : map.spawns) {
		if (Tile* tile = map.getTile(position); !tile || !tile->spawn) {
			++report.stale_spawn_refs;
		}
	}

	for (const auto& [house_id, house] : map.houses) {
		(void)house_id;
		if (!house || map.towns.getTown(house->townid) == nullptr) {
			++report.orphan_houses;
		}
	}

	for (const auto& [waypoint_name, waypoint] : map.waypoints.waypoints) {
		(void)waypoint_name;
		if (!waypoint || !waypoint->pos.isValid() || map.getTile(waypoint->pos) == nullptr) {
			++report.orphan_waypoints;
		}
	}

	return report;
}

std::vector<std::string> buildAuditSummary(const MapAuditReport& report) {
	return {
		std::format("Total tiles scanned: {}", report.total_tiles),
		std::format("Invalid zone tiles: {}", report.invalid_zone_tiles),
		std::format("Orphan house tiles: {}", report.orphan_house_tiles),
		std::format("Houses without valid town: {}", report.orphan_houses),
		std::format("Stale spawn references: {}", report.stale_spawn_refs),
		std::format("Missing spawn references: {}", report.missing_spawn_refs),
		std::format("Waypoints pointing to missing tiles: {}", report.orphan_waypoints)
	};
}

void reconcileSpawnMetadata(Map& map) {
	std::vector<Position> stale_positions;

	for (const auto& position : map.spawns) {
		if (Tile* tile = map.getTile(position); !tile || !tile->spawn) {
			stale_positions.push_back(position);
		}
	}

	for (const Position& stale_position : stale_positions) {
		Position probe = stale_position;
		auto it = map.spawns.find(probe);
		if (it != map.spawns.end()) {
			map.spawns.erase(it);
		}
	}

	for (auto& tile_location : map) {
		Tile* tile = tile_location.get();
		if (!tile || !tile->spawn) {
			continue;
		}

		Position tile_position = tile->getPosition();
		if (map.spawns.find(tile_position) == map.spawns.end()) {
			map.spawns.addSpawn(tile);
		}
	}
}

void cleanupDanglingWaypoints(Map& map) {
	std::vector<std::string> waypoint_names_to_remove;
	waypoint_names_to_remove.reserve(map.waypoints.size());

	for (const auto& [name, waypoint] : map.waypoints.waypoints) {
		if (!waypoint || !waypoint->pos.isValid() || map.getTile(waypoint->pos) == nullptr) {
			waypoint_names_to_remove.push_back(name);
		}
	}

	for (const std::string& name : waypoint_names_to_remove) {
		map.waypoints.removeWaypoint(name);
	}
}
}

MapActionsHandler::MapActionsHandler(MainFrame* frame) :
	frame(frame) {
}

void MapActionsHandler::OnMapRemoveItems(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	FindItemDialog dialog(frame, "Item Type to Remove");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t itemid = dialog.getResultID();

		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		EditorOperations::RemoveItemCondition condition(itemid);
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), condition, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";

		g_gui.SetStatusText(msg);
		g_gui.GetCurrentMap().doChange();
		g_gui.RefreshView();
	}
	dialog.Destroy();
}

void MapActionsHandler::OnMapRemoveCorpses(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = DialogUtil::PopupDialog("Remove Corpses", "Do you want to remove all corpses from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		EditorOperations::RemoveCorpsesCondition func;
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), func, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";
		g_gui.SetStatusText(msg);
		g_gui.GetCurrentMap().doChange();
	}
}

void MapActionsHandler::OnMapRemoveUnreachable(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = DialogUtil::PopupDialog("Remove Unreachable Tiles", "Do you want to remove all unreachable items from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		EditorOperations::RemoveUnreachableCondition func;
		g_gui.CreateLoadBar("Searching map for tiles to remove...");

		long long removed = remove_if_TileOnMap(g_gui.GetCurrentMap(), func);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << removed << " tiles deleted.";

		g_gui.SetStatusText(msg);

		g_gui.GetCurrentMap().doChange();
	}
}

void MapActionsHandler::OnClearHouseTiles(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = DialogUtil::PopupDialog(
		"Clear Invalid House Tiles",
		"Are you sure you want to remove all house tiles that do not belong to a house (this action cannot be undone)?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		editor->clearInvalidHouseTiles(true);
	}

	g_gui.RefreshView();
}

void MapActionsHandler::OnClearModifiedState(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = DialogUtil::PopupDialog(
		"Clear Modified State",
		"This will have the same effect as closing the map and opening it again. Do you want to proceed?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		editor->clearModifiedTileState(true);
	}

	g_gui.RefreshView();
}

void MapActionsHandler::OnMapCleanHouseItems(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = DialogUtil::PopupDialog(
		"Clear Moveable House Items",
		"Are you sure you want to remove all items inside houses that can be moved (this action cannot be undone)?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		// editor->removeHouseItems(true);
	}

	g_gui.RefreshView();
}

void MapActionsHandler::OnBorderizeSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->borderizeSelection();
	g_gui.RefreshView();
}

void MapActionsHandler::OnBorderizeMap(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ret = DialogUtil::PopupDialog("Borderize Map", "Are you sure you want to borderize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		g_gui.GetCurrentEditor()->borderizeMap(true);
	}

	g_gui.RefreshView();
}

void MapActionsHandler::OnRandomizeSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->randomizeSelection();
	g_gui.RefreshView();
}

void MapActionsHandler::OnRandomizeMap(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ret = DialogUtil::PopupDialog("Randomize Map", "Are you sure you want to randomize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		g_gui.GetCurrentEditor()->randomizeMap(true);
	}

	g_gui.RefreshView();
}

void MapActionsHandler::OnMapCleanup(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = DialogUtil::PopupDialog("Cleanup invalid tiles", "Do you want to remove all invalid or unresolved items from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentMap().cleanInvalidTiles(true);
		g_gui.RefreshView();
	}
}

void MapActionsHandler::OnMapCleanInvalidZones(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = DialogUtil::PopupDialog("Cleanup invalid zones", "Do you want to remove all invalid tile flags and opaque OTBM tile fragments from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentMap().cleanInvalidZones(true);
		g_gui.RefreshView();
	}
}

void MapActionsHandler::OnMapAuditAndFix(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	Map& map = g_gui.GetCurrentMap();
	const MapAuditReport before = auditMap(map);
	DialogUtil::ListDialog("Map Audit Report", buildAuditSummary(before));

	const int apply_fixes = DialogUtil::PopupDialog(
		"Apply Quick Fixes",
		"Apply automatic fixes for invalid tiles/zones, house references, spawn metadata, and dangling waypoints?",
		wxYES | wxNO
	);

	if (apply_fixes != wxID_YES) {
		return;
	}

	g_gui.CreateLoadBar("Applying map audit fixes...");
	map.cleanInvalidTiles(false);
	map.cleanInvalidZones(false);
	editor->clearInvalidHouseTiles(false);
	reconcileSpawnMetadata(map);
	cleanupDanglingWaypoints(map);
	g_gui.DestroyLoadBar();

	map.doChange();
	g_gui.RefreshView();

	const MapAuditReport after = auditMap(map);
	std::vector<std::string> result = {
		"Quick-fix completed.",
		std::format("Invalid zone tiles: {} -> {}", before.invalid_zone_tiles, after.invalid_zone_tiles),
		std::format("Orphan house tiles: {} -> {}", before.orphan_house_tiles, after.orphan_house_tiles),
		std::format("Houses without valid town: {} -> {}", before.orphan_houses, after.orphan_houses),
		std::format("Stale spawn references: {} -> {}", before.stale_spawn_refs, after.stale_spawn_refs),
		std::format("Missing spawn references: {} -> {}", before.missing_spawn_refs, after.missing_spawn_refs),
		std::format("Waypoints pointing to missing tiles: {} -> {}", before.orphan_waypoints, after.orphan_waypoints)
	};

	DialogUtil::ListDialog("Map Audit Result", result);
}
