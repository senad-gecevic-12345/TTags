#pragma once
#include "wx/wx.h"
#include "C_Main.h"

class C_App : public wxApp
{

protected:
	C_Main* frame_1{nullptr};
public:
	C_App();
	virtual bool OnInit();
};
 