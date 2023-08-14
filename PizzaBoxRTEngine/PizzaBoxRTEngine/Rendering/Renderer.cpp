#include "Renderer.h"

#include <stdio.h>

namespace PBEngine
{
    extern App app;

#pragma region Renderer definitions
    Renderer::Renderer() {
        renderingBackend = new Backend_FullRT();
        if (!renderingBackend->Init()) {
            fprintf(stderr, "Trouble loading rendering backend of type: %d",
                renderingBackend->backendType);
        }
    }

    Renderer::~Renderer() {
        renderingBackend->CleanupBackend(); // Cleanup the Backend object
        delete renderingBackend;            // Delete the Backend object
    }
#pragma endregion

#pragma region General backend definitions
    bool Backend::Init() { return false; }
    bool Backend::CleanupBackend() { return false; }
#pragma endregion

#pragma region FullRT backend definitions
    bool Backend_FullRT::Init() {
        /*VkPhysicalDeviceProperties deviceProps{};
        VkPhysicalDeviceProperties* devicePropsPtr = &deviceProps;
        vkGetPhysicalDeviceProperties(app.g_PhysicalDevice, devicePropsPtr);
        if ((*devicePropsPtr).limits.maxComputeSharedMemorySize <= 0)
        {
            fprintf(stdin, "This GPU doesn't support raytracing. maxComputeSharedMemorySize: %d",
                (*devicePropsPtr).limits.maxComputeSharedMemorySize);
            return false;
        }
        delete devicePropsPtr;*/

        // Create the command pool
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = app.g_QueueFamily;
        poolCreateInfo.flags = 0; // Optional flags, such as VK_COMMAND_POOL_CREATE_TRANSIENT_BIT or VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

        VkResult result = vkCreateCommandPool(app.g_Device, &poolCreateInfo, nullptr, &commandPool);
        if (result != VK_SUCCESS) {
            // Handle command pool creation failure
            fprintf(stdout, "Couldn't create Command Pool: %d\n", result);
            return false;
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Primary command buffer
        commandBufferAllocateInfo.commandBufferCount = 1; // Allocate a single command buffer
        {
            VkResult result = vkAllocateCommandBuffers(app.g_Device, &commandBufferAllocateInfo, &commandBuffer);

            if (result != VK_SUCCESS) {
                // Handle command buffer allocation error
                printf("Failed to allocate command buffer: %d\n", result);
            }
        }


        return true;
    }

    bool Backend_FullRT::CleanupBackend()
    {
        vkFreeCommandBuffers(app.g_Device, commandPool, 1, &commandBuffer);
        // Unhandled exception at 0x00007FFB3743A215 (vulkan-1.dll) in PizzaBox.exe: Fatal program exit requested.
        vkDestroyCommandPool(app.g_Device, commandPool, nullptr);
        
        return true;
    }
#pragma endregion
}
