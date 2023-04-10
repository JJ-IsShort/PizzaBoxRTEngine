#include "Renderer.h"

#pragma region Renderer definitions
Renderer::Renderer()
{
	renderingBackend = new OptiX();
}

Renderer::~Renderer()
{
	delete renderingBackend; // Delete the Backend object
}
#pragma endregion

#pragma region General backend definitions
bool Backend::Init()
{
	return false;
}
#pragma endregion

#pragma region OptiX backend definitions
bool OptiX::Init()
{
	return false;
}
#pragma endregion