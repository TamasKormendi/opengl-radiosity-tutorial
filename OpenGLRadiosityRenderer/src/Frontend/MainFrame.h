#ifndef MAINFRAME_H
#define MAINFRAME_H

class Renderer;

class MainFrame : public wxFrame {

public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:

	std::unique_ptr<Renderer> renderer;

	void OnLaunchRenderer(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

#endif
