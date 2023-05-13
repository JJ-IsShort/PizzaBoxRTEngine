#include "Renderer.h"

#pragma region Renderer definitions
Renderer::Renderer()
{
	renderingBackend = new Vulkan();
}

Renderer::~Renderer()
{
	renderingBackend->CleanupBackend(); //	Cleanup the Backend object
	delete renderingBackend; //				Delete the Backend object
}
#pragma endregion

#pragma region General backend definitions
bool Backend::Init()
{
	return false;
}
bool Backend::CleanupBackend()
{
	return false;
}
#pragma endregion

#pragma region Vulkan backend definitions
bool Vulkan::Init()
{
	return false;
}

bool Vulkan::CleanupBackend()
{
	return false;
}
#pragma endregion