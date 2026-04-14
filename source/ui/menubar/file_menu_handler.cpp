#include "ui/menubar/file_menu_handler.h"
#include "app/application.h"
#include "app/main.h"
#include "ui/gui.h"
#include "ui/map/export_tilesets_window.h"
#include "ui/dialogs/missing_items_dialog.h"

#include "ui/map/import_map_window.h"
#include "ui/dialog_util.h"
#include "ui/about_window.h"
#include "ui/dat_debug_view.h"
#include "app/preferences.h"
#include "app/settings.h"
#include "ui/extension_window.h"
#include "game/creatures.h"
#include "app/managers/version_manager.h"
#include "ui/controls/sortable_list_box.h"
#include <wx/dirdlg.h>
#include <wx/filename.h>

FileMenuHandler::FileMenuHandler(MainFrame* frame, MainMenuBar* menubar) :
	frame(frame), menubar(menubar) {
}

FileMenuHandler::~FileMenuHandler() = default;

void FileMenuHandler::OnNew(wxCommandEvent& WXUNUSED(event)) {
	g_gui.NewMap();
}

void FileMenuHandler::OnOpen(wxCommandEvent& WXUNUSED(event)) {
	g_gui.OpenMap();
}

void FileMenuHandler::OnSave(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SaveMap();
}

void FileMenuHandler::OnSaveAs(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SaveMapAs();
}

void FileMenuHandler::OnClose(wxCommandEvent& WXUNUSED(event)) {
	frame->DoQuerySave(true); // It closes the editor too
}

void FileMenuHandler::OnQuit(wxCommandEvent& WXUNUSED(event)) {
	g_gui.root->Close();
}

void FileMenuHandler::OnGenerateMap(wxCommandEvent& WXUNUSED(event)) {
	// Not implemented in original code
}

void FileMenuHandler::OnImportMap(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(g_gui.GetCurrentEditor());
	wxDialog* importmap = newd ImportMapWindow(frame, *g_gui.GetCurrentEditor());
	importmap->ShowModal();
	importmap->Destroy();
}

void FileMenuHandler::OnImportMonsterData(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog dlg(g_gui.root, "Import monster/npc file", "", "", "*.xml", wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dlg.GetPaths(paths);
		for (uint32_t i = 0; i < paths.GetCount(); ++i) {
			wxString error;
			std::vector<std::string> warnings;
			bool ok = g_creatures.importXMLFromOT(FileName(paths[i]), error, warnings);
			if (ok) {
				DialogUtil::ListDialog("Monster loader errors", warnings);
			} else {
				wxMessageBox("Error OT data file \"" + paths[i] + "\".\n" + error, "Error", wxOK | wxICON_INFORMATION, g_gui.root);
			}
		}
	}
}

void FileMenuHandler::OnImportMinimap(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(g_gui.IsEditorOpen());
	if (!g_gui.GetCurrentMapTab()) {
		return;
	}

	wxString export_directory = wxstr(g_settings.getString(Config::MINIMAP_EXPORT_DIR));
	if (export_directory.empty()) {
		export_directory = wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	}
	if (export_directory.empty()) {
		export_directory = wxGetCwd();
	}

	wxDirDialog dialog(g_gui.root, "Select export directory for minimap screenshot", export_directory, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	const wxFileName export_dir_info = wxFileName::DirName(dialog.GetPath());
	export_directory = export_dir_info.GetFullPath();
	g_settings.setString(Config::MINIMAP_EXPORT_DIR, nstr(export_directory));

	const bool previous_show_as_minimap = g_settings.getBoolean(Config::SHOW_AS_MINIMAP);
	const bool previous_show_only_colors = g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS);
	const bool previous_show_extra = g_settings.getBoolean(Config::SHOW_EXTRA);

	g_settings.setInteger(Config::SHOW_AS_MINIMAP, true);
	g_settings.setInteger(Config::SHOW_ONLY_TILEFLAGS, true);
	g_settings.setInteger(Config::SHOW_EXTRA, false);
	g_gui.UpdateMenubar();
	g_gui.RefreshView();

	g_gui.GetCurrentMapTab()->GetView()->GetCanvas()->TakeScreenshot(export_dir_info, "png", false);

	// Restore normal viewport flags after scheduling capture.
	frame->CallAfter([previous_show_as_minimap, previous_show_only_colors, previous_show_extra]() {
		g_settings.setInteger(Config::SHOW_AS_MINIMAP, previous_show_as_minimap);
		g_settings.setInteger(Config::SHOW_ONLY_TILEFLAGS, previous_show_only_colors);
		g_settings.setInteger(Config::SHOW_EXTRA, previous_show_extra);
		g_gui.UpdateMenubar();
		g_gui.RefreshView();
	});
}

void FileMenuHandler::OnExportTilesets(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		ExportTilesetsWindow dlg(frame, *g_gui.GetCurrentEditor());
		dlg.ShowModal();
		dlg.Destroy();
	}
}

void FileMenuHandler::OnReloadDataFiles(wxCommandEvent& WXUNUSED(event)) {
	wxString error;
	std::vector<std::string> warnings;
	g_version.LoadVersion(g_version.GetCurrentVersionID(), error, warnings, true);
	DialogUtil::PopupDialog("Error", error, wxOK);
	DialogUtil::ListDialog("Warnings", warnings);
}

void FileMenuHandler::OnMissingItemsReport(wxCommandEvent& WXUNUSED(event)) {
	const auto& missing = g_version.getLastMissingItems();
	if (missing.missing_in_dat.empty() && missing.missing_in_otb.empty() && missing.xml_no_otb.empty() && missing.otb_no_xml.empty()) {
		wxMessageBox("No missing item definitions were detected for the current client version.",
		             "Missing Items Report", wxOK | wxICON_INFORMATION, g_gui.root);
		return;
	}
	MissingItemsDialog::Show(g_gui.root, missing, g_version.lastLoadHasOtb());
}

void FileMenuHandler::OnPreferences(wxCommandEvent& WXUNUSED(event)) {
	PreferencesWindow dialog(frame);
	dialog.ShowModal();
	dialog.Destroy();
}

void FileMenuHandler::OnListExtensions(wxCommandEvent& WXUNUSED(event)) {
	ExtensionsDialog exts(frame);
	exts.ShowModal();
}

void FileMenuHandler::OnGotoWebsite(wxCommandEvent& WXUNUSED(event)) {
	::wxLaunchDefaultBrowser(__SITE_URL__, wxBROWSER_NEW_WINDOW);
}

void FileMenuHandler::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	AboutWindow about(frame);
	about.ShowModal();
}
