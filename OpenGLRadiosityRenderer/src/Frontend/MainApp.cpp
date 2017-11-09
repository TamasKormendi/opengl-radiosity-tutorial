//Code adapted from http://docs.wxwidgets.org/3.1/overview_helloworld.html

#include "stdafx.h"

#include <wx\wx.h>

#include <Frontend\MainApp.h>
#include <Frontend\MainFrame.h>

bool MainApp::OnInit() {

	MainFrame* frame = new MainFrame("Spark Renderer", wxPoint(50, 50), wxSize(450, 340));
	frame->Show(true);

	return true;
}