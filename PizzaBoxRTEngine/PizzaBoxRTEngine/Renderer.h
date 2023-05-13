#pragma once
#include "Renderer.h"

class Backend
{
public:
	enum RendererBackendType {
		RendererBackendType_None,
		RendererBackendType_Vulkan,
		RendererBackendType_Custom
	};
	virtual bool Init();
	virtual bool CleanupBackend();
	RendererBackendType backendType = RendererBackendType_None;
};

class Vulkan : 
	public Backend
{
public:
	bool Init() override;
	bool CleanupBackend() override;
	RendererBackendType backendType = RendererBackendType_Vulkan;
};

class Renderer
{
public:
	Renderer();
	~Renderer();
	Backend* renderingBackend = new Backend(); // Don't forget to keep an eye on the memory for this. A memory leak here probably wouldn't be too bad but still.
};