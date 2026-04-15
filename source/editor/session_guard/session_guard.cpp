#include "editor/session_guard/session_guard.h"

#include "map/map.h"

#include <filesystem>
#include <fstream>
#include <format>

namespace {
std::filesystem::path lockPathFor(const std::string& map_file_path) {
	const std::filesystem::path map_path = std::filesystem::u8path(map_file_path);
	return map_path.parent_path() / (map_path.filename().string() + ".rme.lock");
}

std::filesystem::path autosavePathFor(const std::string& map_file_path) {
	const std::filesystem::path map_path = std::filesystem::u8path(map_file_path);
	return map_path.parent_path() / (map_path.filename().string() + ".autosave.meta");
}
}

bool SessionGuard::acquire(const std::string& map_file_path, std::string& error) {
	if (map_file_path.empty()) {
		return true;
	}

	const std::filesystem::path lock_path = lockPathFor(map_file_path);
	std::error_code ec;
	if (std::filesystem::exists(lock_path, ec)) {
		error = std::format("Session lock exists: {}", lock_path.string());
		return false;
	}

	std::ofstream stream(lock_path.string(), std::ios::trunc | std::ios::out);
	if (!stream.is_open()) {
		error = std::format("Could not create session lock file: {}", lock_path.string());
		return false;
	}
	stream << "rme-session-lock";
	return true;
}

void SessionGuard::release(const std::string& map_file_path) {
	if (map_file_path.empty()) {
		return;
	}
	std::error_code ec;
	std::filesystem::remove(lockPathFor(map_file_path), ec);
}

void SessionGuard::writeTransactionalAutosave(Map& map, const std::string& map_file_path) {
	if (map_file_path.empty()) {
		return;
	}

	const std::filesystem::path autosave_path = autosavePathFor(map_file_path);
	std::ofstream stream(autosave_path.string(), std::ios::trunc | std::ios::out);
	if (!stream.is_open()) {
		return;
	}
	stream << "generation=" << map.getGeneration() << "\n";
	stream << "tiles=" << map.getTileCount() << "\n";
	stream << "changed=" << (map.hasChanged() ? "1" : "0") << "\n";
}
