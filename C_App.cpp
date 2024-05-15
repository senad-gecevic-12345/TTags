#include "C_App.h"
wxIMPLEMENT_APP(C_App); 


C_App::C_App() : wxApp()
{

}

bool C_App::OnInit()
{
    frame_1 = new C_Main();
	frame_1->Show();
	return true;
}


