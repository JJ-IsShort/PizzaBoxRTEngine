#include "Panels/Viewport.h"
#include "Panel.h"

Viewport::Viewport() {}

void Viewport::Show() {
  ImGui::Begin("Viewport");
  ImGui::End();
}
