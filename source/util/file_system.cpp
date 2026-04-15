//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "util/file_system.h"
// #include "utility/file_name.h"
#include "app/settings.h"
#include <wx/stdpaths.h>
#include <wx/app.h>
#ifdef __WINDOWS__
#include <windows.h>
#endif

namespace {
FileName getExecutablePathSafe() {
#ifdef __WINDOWS__
	// wxStandardPaths requires wxApp; during static startup, resolve path via WinAPI instead.
	if (wxTheApp == nullptr) {
		wchar_t modulePath[MAX_PATH] = { 0 };
		const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		if (length > 0) {
			return FileName(wxString(modulePath));
		}
	}
#endif
	return FileName(wxStandardPaths::Get().GetExecutablePath());
}

FileName getUserDataDirSafe() {
	if (wxTheApp != nullptr) {
		return FileName(wxStandardPaths::Get().GetUserDataDir());
	}

	FileName fallback = getExecutablePathSafe();
	fallback.AppendDir("user");
	return fallback;
}
}

wxString FileSystem::m_dataDirectory;

wxString FileSystem::GetDataDirectory() {
	std::string cfg_str = g_settings.getString(Config::DATA_DIRECTORY);
	if (!cfg_str.empty()) {
		FileName dir;
		dir.Assign(wxstr(cfg_str));
		wxString path;
		if (dir.DirExists()) {
			path = dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			return path;
		}
	}

	// Silently reset directory
	FileName exec_directory = getExecutablePathSafe();

	exec_directory.AppendDir("data");
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString FileSystem::GetExecDirectory() {
	// Silently reset directory
	FileName exec_directory = getExecutablePathSafe();
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString FileSystem::GetLocalDataDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory();
		dir.AppendDir("user");
		dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	} else {
		FileName dir = getUserDataDirSafe();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

wxString FileSystem::GetLocalDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory();
		dir.AppendDir("user");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	} else {
		FileName dir = getUserDataDirSafe();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

wxString FileSystem::GetExtensionsDirectory() {
	std::string cfg_str = g_settings.getString(Config::EXTENSIONS_DIRECTORY);
	if (!cfg_str.empty()) {
		FileName dir;
		dir.Assign(wxstr(cfg_str));
		wxString path;
		if (dir.DirExists()) {
			path = dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			return path;
		}
	}

	// Silently reset directory
	FileName local_directory = GetLocalDirectory();
	local_directory.AppendDir("extensions");
	local_directory.Mkdir(0755, wxPATH_MKDIR_FULL);
	return local_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

void FileSystem::DiscoverDataDirectory(const wxString& existentFile) {
	wxString currentDir = wxGetCwd();
	wxString execDir = GetExecDirectory();

	wxString possiblePaths[] = {
		execDir,
		currentDir + "/",

		// these are used usually when running from build directories
		execDir + "/../",
		execDir + "/../../",
		execDir + "/../../../",
		currentDir + "/../",
	};

	bool found = false;
	for (const wxString& path : possiblePaths) {
		if (wxFileName(path + "data/" + existentFile).FileExists()) {
			m_dataDirectory = path + "data/";
			found = true;
			break;
		}
	}

	if (!found) {
		wxLogError(wxString() + "Could not find data directory.\n");
	}
}

wxString FileSystem::GetFoundDataDirectory() {
	return m_dataDirectory;
}
