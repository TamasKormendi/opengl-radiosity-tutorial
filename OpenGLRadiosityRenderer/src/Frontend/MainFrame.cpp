//Code adapted from http://docs.wxwidgets.org/3.1/overview_helloworld.html

#include "stdafx.h"

#include <wx\wx.h>


#include <Renderer\Renderer.h>

#include <Frontend\MainFrame.h>


#include <iostream>
#include <memory>


enum {
	ID_LAUNCH_RENDERER_BUTTON = 1,
	ID_QUIT_BUTTON = 2,

	ID_FILESELECTOR_BUTTON = 3
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
	//EVT_MENU(ID_LAUNCH_RENDERER, MainFrame::OnLaunchRenderer)
	EVT_BUTTON(ID_LAUNCH_RENDERER_BUTTON, MainFrame::OnLaunchRenderer)
	EVT_BUTTON(ID_QUIT_BUTTON, MainFrame::OnExit)
	EVT_BUTTON(ID_FILESELECTOR_BUTTON, MainFrame::OpenFileSelector)

	EVT_MENU(wxID_EXIT, MainFrame::OnExit)
	EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
			: wxFrame(NULL, wxID_ANY, title, pos, size) {
	
	/* wxMenu* menuFile = new wxMenu();

	menuFile->Append(ID_LAUNCH_RENDERER, "Launch Renderer window", "Help");

	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu* menuHelp = new wxMenu();
	menuHelp->Append(wxID_ABOUT);

	wxMenuBar* menuBar = new wxMenuBar();

	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");

	SetMenuBar(menuBar); */


	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	//wxButton* startButton = new wxButton(this, ID_LAUNCH_RENDERER_BUTTON, _T("Launch Renderer"), 0, 0, 0);
	//wxButton* quitButton = new wxButton(this, ID_QUIT_BUTTON, _T("Quit"), 0, 0, 0);

	sizer->Add(new wxButton(this, ID_LAUNCH_RENDERER_BUTTON, "Start renderer", wxDefaultPosition, wxSize(300, 25), 0), 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(new wxButton(this, ID_FILESELECTOR_BUTTON, "Select object file"), 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(new wxButton(this, ID_QUIT_BUTTON, "Quit"), 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(sizer);

	renderer.reset(new Renderer());
}

void MainFrame::OnExit(wxCommandEvent& event) {
	Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& event) {
	wxMessageBox("Spark Radiosity Renderer", "About Spark", wxOK | wxICON_INFORMATION);
}

void MainFrame::OnLaunchRenderer(wxCommandEvent& event) {
	Show(false);

	renderer->startRenderer(filepath);

	std::cout << "Renderer Launched";

	Close(true);
}

void MainFrame::OpenFileSelector(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog* OpenDialog = new wxFileDialog(
		this, _("Choose an object file to open"), wxEmptyString, wxEmptyString,
		_("Wavefront object files (*.obj)|*.obj"),
			wxFD_OPEN, wxDefaultPosition);

	if (OpenDialog->ShowModal() == wxID_OK) // if the user click "Open" instead of "Cancel"
	{
		wxString wxFilepath = OpenDialog->GetPath();

		filepath = wxFilepath.ToStdString();

		std::replace(filepath.begin(), filepath.end(), '\\', '/');

		std::cout << filepath << std::endl;
	}

	OpenDialog->Destroy();
}