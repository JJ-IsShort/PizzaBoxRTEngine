#include "Rendering/Renderer.h"
#include <stdio.h>

#pragma region Renderer definitions
Renderer::Renderer() {
  renderingBackend = new Backend_FullRT();
  if (renderingBackend->Init()) {
    fprintf(stderr, "Trouble loading rendering backend of type: %d",
            renderingBackend->backendType);
  }
}

Renderer::~Renderer() {
  renderingBackend->CleanupBackend(); //	Cleanup the Backend object
  delete renderingBackend;            //				Delete the Backend
                                      // object
}
#pragma endregion

#pragma region General backend definitions
bool Backend::Init() { return false; }
bool Backend::CleanupBackend() { return false; }
#pragma endregion

#pragma region FullRT backend definitions
bool Backend_FullRT::Init() { return false; }

bool Backend_FullRT::CleanupBackend() { return false; }
#pragma endregion
