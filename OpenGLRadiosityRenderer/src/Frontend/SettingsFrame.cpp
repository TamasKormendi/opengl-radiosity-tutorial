#include "stdafx.h"

#include <wx\wx.h>

#include <Frontend\SettingsFrame.h>

#include <iostream>
#include <memory>

enum {
	ID_RENDERER_RESOLUTION = 1,
	ID_LIGHTMAP_RESOLUTION = 2,
	ID_ATTENUATION_TYPE = 3
	


};

wxBEGIN_EVENT_TABLE(SettingsFrame, wxFrame)
	EVT_CHOICE(ID_RENDERER_RESOLUTION, SettingsFrame::OnRendererResolutionSelected)
	EVT_CHOICE(ID_LIGHTMAP_RESOLUTION, SettingsFrame::OnLightmapResolutionSelected)
	EVT_CHOICE(ID_RENDERER_RESOLUTION, SettingsFrame::OnAttenuationSelected)

	EVT_CLOSE(SettingsFrame::OnClose)
wxEND_EVENT_TABLE()

SettingsFrame::SettingsFrame(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(parent, wxID_ANY, title, pos, size) {

	//Might change these to enums but want to try how drop-down menus work first

	rendererResolution = 1;
	lightmapResolution = 0;
	attenuationType = 1;

	continuousUpdate = true;

	textureFiltering = true;
	multisampling = true;


	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	//Labels
	wxStaticText* rendererResolutionLabel = new wxStaticText(this, -1, "Renderer resolution:");
	wxStaticText* lightmapResolutionLabel = new wxStaticText(this, -1, "Lightmap resolution:");
	wxStaticText* attenuationLabel = new wxStaticText(this, -1, "Attenuation type:");


	//Options
	wxArrayString rendererResolutionOptions;
	rendererResolutionOptions.Add(wxT("800x800"));
	rendererResolutionOptions.Add(wxT("1280x720"));
	rendererResolutionOptions.Add(wxT("1920x1080"));

	wxArrayString lightmapResolutionOptions;
	lightmapResolutionOptions.Add(wxT("32x32"));
	lightmapResolutionOptions.Add(wxT("64x64"));
	lightmapResolutionOptions.Add(wxT("128x128"));
	lightmapResolutionOptions.Add(wxT("256x256"));
	lightmapResolutionOptions.Add(wxT("512x512"));

	wxArrayString attenuationOptions;
	attenuationOptions.Add(wxT("Linear"));
	attenuationOptions.Add(wxT("Quadratic"));
	attenuationOptions.Add(wxT("Quadratic with patch size correction"));


	//Object creation
	rendererResolutionChoice = new wxChoice(this, 1, wxDefaultPosition, wxDefaultSize, rendererResolutionOptions, wxTE_RICH | wxTE_CENTRE );
	rendererResolutionChoice->SetSelection(rendererResolution);

	lightmapChoice = new wxChoice(this, 2, wxDefaultPosition, wxDefaultSize, lightmapResolutionOptions, wxTE_RICH | wxTE_CENTRE);
	lightmapChoice->SetSelection(lightmapResolution);

	attenuationChoice = new wxChoice(this, 3, wxDefaultPosition, wxDefaultSize, attenuationOptions, wxTE_RICH | wxTE_CENTRE);
	attenuationChoice->SetSelection(attenuationType);


	//Add to sizer
	sizer->Add(rendererResolutionLabel, 0, wxALIGN_CENTER | wxALL);
	sizer->Add(rendererResolutionChoice, 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(lightmapResolutionLabel, 0, wxALIGN_CENTER | wxALL);
	sizer->Add(lightmapChoice, 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(attenuationLabel, 0, wxALIGN_CENTER | wxALL);
	sizer->Add(attenuationChoice, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(sizer);

}

void SettingsFrame::OnRendererResolutionSelected(wxCommandEvent& event) {
	rendererResolution = rendererResolutionChoice->GetSelection();
}

void SettingsFrame::OnLightmapResolutionSelected(wxCommandEvent& event) {
	lightmapResolution = lightmapChoice->GetSelection();
}

void SettingsFrame::OnAttenuationSelected(wxCommandEvent& event) {
	attenuationType = attenuationChoice->GetSelection();
}

void SettingsFrame::OnClose(wxCloseEvent& closeEvent) {
	Show(false);
}
