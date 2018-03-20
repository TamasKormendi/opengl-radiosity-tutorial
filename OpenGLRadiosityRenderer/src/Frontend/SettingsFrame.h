#ifndef SETTINGSFRAME_H
#define SETTINGSFRAME_H

class SettingsFrame : public wxFrame {

public:
	wxChoice* rendererResolutionChoice;
	int rendererResolution;

	wxChoice* lightmapChoice;
	int lightmapResolution;

	wxChoice* attenuationChoice;
	int attenuationType;

	wxCheckBox* updateBox;
	bool continuousUpdate;

	wxCheckBox* filteringBox;
	bool textureFiltering;

	wxCheckBox* multisamplingBox;
	bool multisampling;

	SettingsFrame(wxWindow * parent, const wxString& title, const wxPoint& pos, const wxSize& size);

private:

	void OnRendererResolutionSelected(wxCommandEvent& event);
	void OnLightmapResolutionSelected(wxCommandEvent& event);
	void OnAttenuationSelected(wxCommandEvent& event);

	void OnUpdateChecked(wxCommandEvent& event);
	void OnFilteringChecked(wxCommandEvent& event);
	void OnMultisamplingChecked(wxCommandEvent& event);

	void OnClose(wxCloseEvent& closeEvent);

	wxDECLARE_EVENT_TABLE();
};


#endif 
