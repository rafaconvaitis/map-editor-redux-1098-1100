#include "ui/menubar/map_actions_handler.h"
#include "ui/gui.h"
#include "ui/dialog_util.h"
#include "ui/find_item_window.h"
#include "editor/editor.h"
#include "editor/operations/clean_operations.h"
#include "editor/operations/search_operations.h"
#include "map/map.h"
#include "map/map_health_score.h"
#include "editor/action_queue.h"
#include "worldgen/prompt_layout_engine.h"
#include "worldgen/moonshot_service.h"
#include <format>
#include <vector>
#include <wx/textdlg.h>
#include <wx/datetime.h>

namespace {
MoonshotWorldgenService g_moonshot_worldgen_service;

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

std::vector<std::string> buildMoonshotReportLines(const WorldgenExecutionReport& report) {
	std::vector<std::string> lines;
	lines.push_back(std::format("Preset: {}", report.request.preset.display_name));
	lines.push_back(std::format(
		"Variation: {} ({}/{})",
		report.variation_signature.empty() ? report.request.variation_signature : report.variation_signature,
		report.variation_index,
		report.variation_space
	));
	lines.push_back(std::format(
		"Region: ({}, {}, {}) -> ({}, {}, {})",
		report.request.region.x1,
		report.request.region.y1,
		report.request.region.z,
		report.request.region.x2,
		report.request.region.y2,
		report.request.region.z
	));
	lines.push_back(std::format("Prompt: {}", report.request.prompt));
	lines.emplace_back("");
	lines.emplace_back("Validation");
	for (const auto& line : report.validation.messages) {
		lines.push_back(std::format(" - {}", line));
	}
	lines.emplace_back("");
	lines.emplace_back("Semantic Diff");
	for (const auto& line : report.semantic_diff.messages) {
		lines.push_back(std::format(" - {}", line));
	}
	lines.emplace_back("");
	lines.emplace_back("Performance");
	for (const auto& sample : report.performance) {
		lines.push_back(std::format(" - {}: {:.2f} ms", sample.stage, sample.milliseconds));
	}
	lines.emplace_back("");
	lines.emplace_back("Timeline (last runs)");
	for (const auto& entry : report.timeline_tail) {
		lines.push_back(std::format(
			" - [{}] {} | {} | tiles={}",
			entry.timestamp,
			entry.preset_name,
			entry.prompt,
			entry.snapshot.tile_count
		));
	}
	return lines;
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
	editor->beginSessionOperation("Map Audit Quick Fix");
	const size_t checkpoint = editor->createCheckpoint("Before Map Audit");
	const MapAuditReport before = auditMap(map);
	DialogUtil::ListDialog("Map Audit Report", buildAuditSummary(before));

	const int apply_fixes = DialogUtil::PopupDialog(
		"Apply Quick Fixes",
		"Apply automatic fixes for invalid tiles/zones, house references, spawn metadata, and dangling waypoints?",
		wxYES | wxNO
	);

	if (apply_fixes != wxID_YES) {
		editor->endSessionOperation();
		return;
	}

	g_gui.CreateLoadBar("Applying map audit fixes...");
	map.cleanInvalidTiles(false);
	map.cleanInvalidZones(false);
	editor->clearInvalidHouseTiles(false);
	reconcileSpawnMetadata(map);
	cleanupDanglingWaypoints(map);
	g_gui.DestroyLoadBar();
	editor->endSessionOperation();

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
	const MapHealthScoreReport health = MapHealthScorer::evaluate(map);
	result.emplace_back("");
	result.emplace_back("Map health:");
	for (const std::string& line : health.lines) {
		result.push_back(std::format("  {}", line));
	}
	result.emplace_back("");
	result.emplace_back("Recent action timeline:");
	const auto timeline = editor->actionQueue->buildTimeline(12);
	for (const std::string& row : timeline) {
		result.push_back(std::format("  {}", row));
	}
	result.push_back(std::format("Checkpoint id: {}", checkpoint));

	DialogUtil::ListDialog("Map Audit Result", result);
}

void MapActionsHandler::OnMapMoonshotStudio(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	if (editor->selection.empty()) {
		DialogUtil::PopupDialog("Moonshot Worldgen", "Select an area first. The Moonshot generator uses selection bounds as the target region.", wxOK | wxICON_INFORMATION);
		return;
	}

	wxTextEntryDialog prompt_dialog(
		frame,
		"Describe what you want to generate (examples: medieval city with roads, undead dungeon, hunt forest for level 80-120).",
		"Moonshot Prompt-to-Layout"
	);
	if (prompt_dialog.ShowModal() != wxID_OK) {
		return;
	}

	const std::string prompt_text = prompt_dialog.GetValue().ToStdString();
	if (prompt_text.empty()) {
		DialogUtil::PopupDialog("Moonshot Worldgen", "Prompt cannot be empty.", wxOK | wxICON_WARNING);
		return;
	}

	PromptLayoutEngine prompt_engine;
	const PromptDraft draft = prompt_engine.buildDraft(prompt_text);
	const int decision = DialogUtil::PopupDialog(
		"Moonshot Draft",
		std::format(
			"Detected preset: {}\nConfidence: {:.0f}%\n{}\n\nApply to selected region?",
			draft.preset.display_name,
			draft.confidence * 100.0,
			draft.summary
		),
		wxYES | wxNO | wxCANCEL
	);
	if (decision == wxID_CANCEL) {
		return;
	}

	Position min = editor->selection.minPosition();
	Position max = editor->selection.maxPosition();
	WorldgenRegion region {
		.x1 = std::min(min.x, max.x),
		.y1 = std::min(min.y, max.y),
		.x2 = std::max(min.x, max.x),
		.y2 = std::max(min.y, max.y),
		.z = min.z
	};

	const bool partial_apply = decision == wxID_NO;
	if (partial_apply) {
		region.x2 = region.x1 + std::max(1, region.width() / 2);
		region.y2 = region.y1 + std::max(1, region.height() / 2);
	}

	const uint64_t seed = static_cast<uint64_t>(wxGetUTCTimeMillis().GetValue());
	WorldgenRequest request {
		.seed = seed,
		.preset = draft.preset,
		.region = region,
		.prompt = prompt_text,
		.partial_apply = partial_apply
	};

	g_gui.CreateLoadBar("Moonshot worldgen executing...");
	WorldgenExecutionReport report = g_moonshot_worldgen_service.execute(g_gui.GetCurrentMap(), request);
	g_gui.DestroyLoadBar();

	g_gui.SetStatusText(std::format(
		"Moonshot: {} generated in {}x{} region",
		request.preset.display_name,
		request.region.width(),
		request.region.height()
	));
	g_gui.RefreshView();

	DialogUtil::ListDialog("Moonshot Execution Report", buildMoonshotReportLines(report));
}
