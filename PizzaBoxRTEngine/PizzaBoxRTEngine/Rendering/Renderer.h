#pragma once
#define VK_NO_PROTOTYPES
#include "app.h"
//#include "vulkan/vulkan.hpp"

namespace PBEngine
{
    class Backend {
    public:
        enum RendererBackendType {
            RendererBackendType_None,
            RendererBackendType_FullRT,
            RendererBackendType_Custom
        };
        virtual bool Init();
        virtual bool CleanupBackend();
        const RendererBackendType backendType = RendererBackendType_None;
    };

    class Backend_FullRT : public Backend {
    public:
        bool Init() override;
        bool CleanupBackend() override;
        const RendererBackendType backendType = RendererBackendType_FullRT;

        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;
    };

    class Renderer {
    public:
        Renderer();
        ~Renderer();
        Backend* renderingBackend; // Don't forget to keep an eye on the memory for
        // this. A memory leak here probably wouldn't be
        // too bad but still.
    };
}
