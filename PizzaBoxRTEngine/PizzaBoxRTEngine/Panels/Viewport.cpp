#include "Viewport.h"
//#include "Panel.h"

namespace PBEngine
{
	Viewport::Viewport() {}

	void Viewport::Show() {
		ImGui::Begin("Viewport");
		ImGui::End();
	}
	
	void Viewport::Init() {
		renderer.renderingBackend->Init();
	}

	Viewport::~Viewport() {}
}
