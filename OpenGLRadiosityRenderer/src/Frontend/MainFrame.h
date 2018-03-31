#ifndef MAINFRAME_H
#define MAINFRAME_H

class Renderer;
class SettingsFrame;

class MainFrame : public wxFrame {

public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:

	std::unique_ptr<Renderer> renderer;

	std::string filepath;

	wxTextCtrl* filepathBox;

	SettingsFrame* settings;

	void OnLaunchRenderer(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OpenFileSelector(wxCommandEvent& WXUNUSED(event));

	void OpenSettings(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

#endif
