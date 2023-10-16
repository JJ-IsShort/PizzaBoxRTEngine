#include "Panel.h"

namespace PBEngine
{
	void Panel::Show()
	{
		ImGui::Begin("Base Panel Class");
		ImGui::End();
	}
	
	void Panel::PreRender()
	{
		return;
	}

	void Panel::Init()
	{
		return;
	}
}
