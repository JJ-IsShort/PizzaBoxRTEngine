#pragma once
#include "Renderer.h"

class Backend
{
public:
	enum RendererBackendType {
		RendererBackendType_None,
		RendererBackendType_OptiX,
		RendererBackendType_Custom
	};
	virtual bool Init();
	RendererBackendType backendType = RendererBackendType_None;
};

class OptiX : 
	public Backend
{
public:
	bool Init() override;
	RendererBackendType backendType = RendererBackendType_OptiX;
};

class Renderer
{
public:
	Renderer();
	~Renderer();
	Backend* renderingBackend = new Backend(); // Don't forget to keep an eye on the memory for this. A memory leak her probably wouldn't be too bad but still.
};