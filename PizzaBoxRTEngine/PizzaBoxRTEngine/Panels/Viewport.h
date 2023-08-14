#pragma once
#include "Panel.h"
#include "../Rendering/Renderer.h"

namespace PBEngine
{
	class Viewport : public Panel {
	public:
		Viewport();
		void Show() override;
		void Init() override;
		~Viewport() override;
		Renderer renderer;
	};
}
