#include "io/export_pack/export_pack_service.h"

#include "map/map.h"

#include <filesystem>
#include <fstream>
#include <format>
#include <iterator>

bool ExportPackService::writeMetadataPackage(const Map& map, const std::string& output_directory, std::string& error) {
	namespace fs = std::filesystem;
	std::error_code ec;
	const fs::path directory = fs::u8path(output_directory);
	if (!fs::exists(directory, ec) && !fs::create_directories(directory, ec)) {
		error = "Could not create output directory for export pack.";
		return false;
	}

	const fs::path metadata_file = directory / (map.getName() + ".export.meta.txt");
	std::ofstream stream(metadata_file.string(), std::ios::trunc | std::ios::out);
	if (!stream.is_open()) {
		error = "Could not write export metadata file.";
		return false;
	}

	stream << std::format("map_name={}\n", map.getName());
	stream << std::format("width={}\n", map.getWidth());
	stream << std::format("height={}\n", map.getHeight());
	stream << std::format("tile_count={}\n", map.getTileCount());
	stream << std::format("town_count={}\n", map.towns.count());
	stream << std::format("house_count={}\n", map.houses.count());
	stream << std::format("spawn_count={}\n", std::distance(map.spawns.begin(), map.spawns.end()));
	stream << std::format("waypoint_count={}\n", map.waypoints.size());
	return true;
}
