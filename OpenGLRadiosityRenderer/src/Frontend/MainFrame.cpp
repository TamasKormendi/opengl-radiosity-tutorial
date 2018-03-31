//Code loosely adapted from http://docs.wxwidgets.org/3.1/overview_helloworld.html and https://wiki.wxwidgets.org/Writing_Your_First_Application-Common_Dialogs

#include "stdafx.h"

#include <wx\wx.h>


#include <Renderer\Renderer.h>

#include <Frontend\MainFrame.h>
#include <Frontend\SettingsFrame.h>


#include <iostream>
#include <memory>


enum {
	ID_LAUNCH_RENDERER_BUTTON = 1,
	ID_QUIT_BUTTON = 2,

	ID_FILESELECTOR_BUTTON = 3,
	ID_SETTINGS_BUTTON = 4
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_BUTTON(ID_LAUNCH_RENDERER_BUTTON, MainFrame::OnLaunchRenderer)
	EVT_BUTTON(ID_QUIT_BUTTON, MainFrame::OnExit)
	EVT_BUTTON(ID_FILESELECTOR_BUTTON, MainFrame::OpenFileSelector)

	EVT_BUTTON(ID_SETTINGS_BUTTON, MainFrame::OpenSettings)

	EVT_MENU(wxID_EXIT, MainFrame::OnExit)
	EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
			: wxFrame(NULL, wxID_ANY, title, pos, size) {

	settings = new SettingsFrame(this, "Settings", wxPoint(50, 50), wxSize(450, 340));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	filepathBox = new wxTextCtrl(this, -1, "Please select a .obj file", wxDefaultPosition, wxSize(200, 20), wxTE_RICH | wxTE_CENTRE | wxTE_READONLY, wxDefaultValidator, wxTextCtrlNameStr);

	sizer->Add(new wxButton(this, ID_LAUNCH_RENDERER_BUTTON, "Start renderer", wxDefaultPosition, wxSize(300, 25), 0), 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(filepathBox, 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(new wxButton(this, ID_FILESELECTOR_BUTTON, "Select object file"), 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(new wxButton(this, ID_SETTINGS_BUTTON, "Settings"), 0, wxALIGN_CENTER | wxALL, 10);

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

	std::cout << "Attenuation in frame: " << settings->attenuationType << std::endl;

	renderer->setParameters(settings->rendererResolution, settings->lightmapResolution, settings->attenuationType, settings->continuousUpdate, settings->textureFiltering, settings->multisampling);

	renderer->startRenderer(filepath);

	std::cout << "Renderer Launched";

	Close(true);
}

void MainFrame::OpenFileSelector(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog* OpenDialog = new wxFileDialog(
		this, _("Choose an object file to open"), wxEmptyString, wxEmptyString,
		_("Wavefront object files (*.obj)|*.obj"),
			wxFD_OPEN, wxDefaultPosition);

	if (OpenDialog->ShowModal() == wxID_OK) // if the user clicks "Open" instead of "Cancel"
	{
		wxString wxFilepath = OpenDialog->GetPath();

		filepath = wxFilepath.ToStdString();

		std::replace(filepath.begin(), filepath.end(), '\\', '/');

		std::cout << filepath << std::endl;

		filepathBox->ChangeValue(filepath);
	}

	OpenDialog->Destroy();
}

void MainFrame::OpenSettings(wxCommandEvent& event) {
	settings->Show(true);
}