#ifndef MAINFRAME_H
#define MAINFRAME_H

class Renderer;

class MainFrame : public wxFrame {

public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:

	std::unique_ptr<Renderer> renderer;

	std::string filepath;

	wxTextCtrl* filepathBox;

	void OnLaunchRenderer(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OpenFileSelector(wxCommandEvent& WXUNUSED(event));

	wxDECLARE_EVENT_TABLE();
};

#endif
