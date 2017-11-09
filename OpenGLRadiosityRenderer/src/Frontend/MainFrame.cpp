//Code adapted from http://docs.wxwidgets.org/3.1/overview_helloworld.html

#include "stdafx.h"

#include <wx\wx.h>

#include <Renderer\Renderer.h>

#include <Frontend\MainFrame.h>


#include <iostream>

enum {
	ID_LAUNCH_RENDERER = 1
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(ID_LAUNCH_RENDERER, MainFrame::OnLaunchRenderer)
	EVT_MENU(wxID_EXIT, MainFrame::OnExit)
	EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
			: wxFrame(NULL, wxID_ANY, title, pos, size) {
	
	wxMenu* menuFile = new wxMenu();

	menuFile->Append(ID_LAUNCH_RENDERER, "Launch Renderer window", "Help");

	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu* menuHelp = new wxMenu();
	menuHelp->Append(wxID_ABOUT);

	wxMenuBar* menuBar = new wxMenuBar();

	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");

	SetMenuBar(menuBar);

	renderer = new Renderer();
}

void MainFrame::OnExit(wxCommandEvent& event) {
	Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& event) {
	wxMessageBox("Spark Radiosity Renderer", "About Spark", wxOK | wxICON_INFORMATION);
}

void MainFrame::OnLaunchRenderer(wxCommandEvent& event) {
	Show(false);

	renderer->startRenderer();

	std::cout << "Renderer Launched";

	//TODO Make this a smart pointer
	delete(renderer);

	Close(true);
}