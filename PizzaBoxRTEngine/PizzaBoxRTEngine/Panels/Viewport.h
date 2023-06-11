#pragma once
#include "Panel.h"
#include "Rendering/Renderer.h"

class Viewport : public Panel {
public:
  Viewport();
  void Show() override;
  Renderer renderer;
};
