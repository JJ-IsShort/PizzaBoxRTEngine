#pragma once
#include "Panel.h"
#include "../Rendering/Renderer.h"

namespace PBEngine
{
	class Viewport : public Panel {
	public:
		Viewport();
		void Show() override;
		void PreRender() override;
		void Init() override;
		~Viewport() override;
		std::unique_ptr<Renderer> renderer;

		float width;
		float height;
	};
}
