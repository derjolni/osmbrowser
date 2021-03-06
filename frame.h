// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __MAINFRAME_H__
#define __MAINFRAME_H__

#include <wx/frame.h>
#include <wx/string.h>
#include <wx/gauge.h>
#include <wx/app.h>

class RuleControl;
class InfoTreeCtrl;
class OsmCanvas;
class ColorRules;

// Define a new frame type: this is going to be our main frame
class MainFrame : public wxFrame
{
public:
	MainFrame(wxApp *app, const wxString& title, wxString const &fileName);
	~MainFrame();

	// event handlers (these functions should _not_ be virtual)
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnSavePdf(wxCommandEvent &event);
	void OnClose(wxCloseEvent &event);
	void OnSize(wxSizeEvent &event);

    OsmCanvas *m_canvas;
	RuleControl *m_drawRule;
	ColorRules *m_colorRules;
	InfoTreeCtrl *m_info;

	void Save(wxString const &name);
	void Load(wxString const &name);

	void SetProgress(double progress, wxString const &text = wxEmptyString);

private:
	wxGauge *m_progress;
	wxStatusBar *m_statusBar;

	// any class wishing to process wxWidgets events must use this macro
	DECLARE_EVENT_TABLE()

};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
	Menu_Quit = wxID_EXIT,
	Menu_About = wxID_ABOUT,
	Menu_Save_Pdf = wxID_HIGHEST

};




#endif
