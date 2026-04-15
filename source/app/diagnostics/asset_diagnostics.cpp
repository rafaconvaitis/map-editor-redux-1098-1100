#include "app/diagnostics/asset_diagnostics.h"

#include "app/client_version.h"

#include <wx/filename.h>

AssetDiagnosticsReport AssetDiagnostics::analyze(const ClientVersion& version) {
	AssetDiagnosticsReport report;
	const FileName data_path = version.getDataPath();
	const wxFileName sprites_path = version.getSpritesPath();
	const wxFileName metadata_path = version.getMetadataPath();

	if (!data_path.DirExists()) {
		report.ok = false;
		report.issues.push_back("Data directory is missing: " + data_path.GetFullPath().ToStdString());
		report.repair_hints.push_back("Open Preferences -> Client Version and set a valid data directory.");
	}
	if (!sprites_path.FileExists()) {
		report.ok = false;
		report.issues.push_back("Missing .spr file: " + sprites_path.GetFullPath().ToStdString());
		report.repair_hints.push_back("Copy a compatible .spr file for this client build.");
	}
	if (!metadata_path.FileExists()) {
		report.ok = false;
		report.issues.push_back("Missing metadata file: " + metadata_path.GetFullPath().ToStdString());
		report.repair_hints.push_back("Check .dat/.otb/.xml mapping and reload the client version.");
	}

	const uint32_t dat_signature = version.getDatSignature();
	const uint32_t spr_signature = version.getSprSignature();
	if (dat_signature == 0 || spr_signature == 0) {
		report.ok = false;
		report.issues.push_back("Missing signatures for .dat/.spr in this profile.");
		report.repair_hints.push_back("Re-import client files and save version profile.");
	}

	if (report.ok) {
		report.issues.push_back("Assets look healthy for this version.");
	}
	return report;
}
