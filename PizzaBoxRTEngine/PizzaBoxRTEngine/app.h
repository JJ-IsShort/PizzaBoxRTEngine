#pragma once
#ifdef _WIN32
#include "../../External/imgui/imgui.h"
#include "../../External/imgui/backends/imgui_impl_glfw.h"
#include "../../External/imgui/backends/imgui_impl_vulkan.h"
#else
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#endif

#include <iostream>
#include <stdio.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#ifdef _WIN32
#include "../../External/glfw/include/GLFW/glfw3.h"
#else
#include <GLFW/glfw3.h>
#endif
#include "../../External/volk/volk.h"

#include <vector>

// #define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif
#include "Scene/Scene.h"

namespace PBEngine
{
	class App
	{
	public:
		int Start();

        // Vulkan Data
        static VkAllocationCallbacks* g_Allocator;
        static VkInstance g_Instance;
        static VkPhysicalDevice g_PhysicalDevice;
        static VkDevice g_Device;
        static uint32_t g_QueueFamily[2]; // 1 - Rendering, 2 - Compute
        static VkQueue g_RenderQueue[2];
        static VkQueue g_ComputeQueue;
        static VkDebugReportCallbackEXT g_DebugReport;
        static VkPipelineCache g_PipelineCache;
        static VkDescriptorPool g_DescriptorPool;

        static ImGui_ImplVulkanH_Window g_MainWindowData;
        static int g_MinImageCount;
        static bool g_SwapChainRebuild;

        Scene scene;

        void SetupImGuiStyle();
	private:
#pragma region Scary Vulkan Functions
        static void glfw_error_callback(int error, const char* description) {
            fprintf(stderr, "GLFW Error %d: %s\n", error, description);
        }
        static void check_vk_result(VkResult err) {
            if (err == 0)
                return;
            fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
            if (err < 0)
                abort();
        }

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        static VKAPI_ATTR VkBool32 VKAPI_CALL
            debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
                uint64_t object, size_t location, int32_t messageCode,
                const char* pLayerPrefix, const char* pMessage, void* pUserData) {
            (void)flags;
            (void)object;
            (void)location;
            (void)messageCode;
            (void)pUserData;
            (void)pLayerPrefix; // Unused arguments
            fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n",
                objectType, pMessage);
            return VK_FALSE;
        }
#endif // IMGUI_VULKAN_DEBUG_REPORT

        static bool
            IsExtensionAvailable(const std::vector<VkExtensionProperties>& properties,
                const char* extension) {
            for (const VkExtensionProperties& p : properties)
                if (strcmp(p.extensionName, extension) == 0)
                    return true;
            return false;
        }

        static VkPhysicalDevice SetupVulkan_SelectPhysicalDevice() {
            uint32_t gpu_count;
            VkResult err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
            check_vk_result(err);
            IM_ASSERT(gpu_count > 0);

            ImVector<VkPhysicalDevice> gpus;
            gpus.resize(gpu_count);
            err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus.Data);
            check_vk_result(err);

            // If a number >1 of GPUs got reported, find discrete GPU if present, or use
            // first one available. This covers most common cases
            // (multi-gpu/integrated+dedicated graphics). Handling more complicated setups
            // (multiple dedicated GPUs) is out of scope of this sample.
            for (VkPhysicalDevice& device : gpus) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                    return device;
            }
            return VK_NULL_HANDLE;
        }

        static void SetupVulkan(ImVector<const char*> instance_extensions) {
            VkResult err;

            // Create Vulkan Instance
            {
                VkInstanceCreateInfo create_info = {};
                create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

                // Enumerate available extensions
                uint32_t properties_count = 0;
                std::vector<VkExtensionProperties> properties;
                err = vkEnumerateInstanceExtensionProperties(NULL, &properties_count, NULL);
                properties.resize(properties_count);
                err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count,
                    properties.data());
                check_vk_result(err);

                // Enable required extensions
                if (IsExtensionAvailable(
                    properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
                    instance_extensions.push_back(
                        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
                if (IsExtensionAvailable(properties,
                    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
                    instance_extensions.push_back(
                        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
                    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
                }
#endif
                if (IsExtensionAvailable(properties,
                    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
                    instance_extensions.push_back(
                        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                if (IsExtensionAvailable(properties,
                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
                    instance_extensions.push_back(
                        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

                // Enabling validation layers
#ifdef IMGUI_VULKAN_DEBUG_REPORT
                const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
                create_info.enabledLayerCount = 1;
                create_info.ppEnabledLayerNames = layers;
                instance_extensions.push_back("VK_EXT_debug_report");
#endif

                // Create Vulkan Instance
                create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
                create_info.ppEnabledExtensionNames = instance_extensions.Data;
                err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
                check_vk_result(err);
                volkLoadInstance(g_Instance);

                // Setup the debug report callback
#ifdef IMGUI_VULKAN_DEBUG_REPORT
                auto vkCreateDebugReportCallbackEXT =
                    (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                        g_Instance, "vkCreateDebugReportCallbackEXT");
                IM_ASSERT(vkCreateDebugReportCallbackEXT != nullptr);
                VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
                debug_report_ci.sType =
                    VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
                debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                    VK_DEBUG_REPORT_WARNING_BIT_EXT |
                    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
                debug_report_ci.pfnCallback = debug_report;
                debug_report_ci.pUserData = nullptr;
                err = vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci,
                    g_Allocator, &g_DebugReport);
                check_vk_result(err);
#endif
            }

            // Select Physical Device (GPU)
            g_PhysicalDevice = SetupVulkan_SelectPhysicalDevice();

            // Select graphics queue family
            {
                uint32_t count;
                vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
                VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(
                    sizeof(VkQueueFamilyProperties) * count);
                vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
                for (uint32_t i = 0; i < count; i++)
                {
                    if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                        g_QueueFamily[0] = i;
                    }
                    if (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                        g_QueueFamily[1] = i;
                    }
                }
                free(queues);
                IM_ASSERT(g_QueueFamily[0] != (uint32_t)-1);
                IM_ASSERT(g_QueueFamily[1] != (uint32_t)-1);
            }

            // Create Logical Device (with 1 queue)
            {
                ImVector<const char*> device_extensions;
                device_extensions.push_back("VK_KHR_swapchain");
                device_extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                device_extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                device_extensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
                device_extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
                device_extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
                device_extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                device_extensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

                // Enumerate physical device extension
                uint32_t properties_count;
                ImVector<VkExtensionProperties> properties;
                vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr,
                    &properties_count, nullptr);
                properties.resize(properties_count);
                vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr,
                    &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
                if (IsExtensionAvailable(properties,
                    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
                    device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif
                VkPhysicalDeviceFeatures2 enabledFeatures = {};

                VkPhysicalDeviceBufferDeviceAddressFeaturesKHR addrFeatures = {};
                addrFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
                addrFeatures.pNext = nullptr;
                addrFeatures.bufferDeviceAddress = VK_TRUE;

                // Enable ray tracing pipeline feature
                VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {};
                rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
                rayTracingFeatures.pNext = &addrFeatures;
                rayTracingFeatures.rayTracingPipeline = VK_TRUE;

                // Enable acceleration structure feature
                VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructureFeatures = {};
                accelStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
                accelStructureFeatures.pNext = &rayTracingFeatures;
                accelStructureFeatures.accelerationStructure = VK_TRUE;

                // Add the features to the enabledFeatures structure
                enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                enabledFeatures.pNext = &accelStructureFeatures;

                VkDeviceQueueCreateInfo queue_info[2] = {};
                const float queuePriorityRender[] = { 0.0f, 1.0f };
                queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[0].queueFamilyIndex = g_QueueFamily[0];
                queue_info[0].queueCount = 2;
                queue_info[0].pQueuePriorities = queuePriorityRender;
                
                const float queuePriorityCompute[] = { 1.0f };
                queue_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[1].queueFamilyIndex = g_QueueFamily[1];
                queue_info[1].queueCount = 1;
                queue_info[1].pQueuePriorities = queuePriorityCompute;

                VkDeviceCreateInfo create_info = {};
                create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                create_info.queueCreateInfoCount =
                    sizeof(queue_info) / sizeof(queue_info[0]);
                create_info.pQueueCreateInfos = queue_info;
                create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
                create_info.ppEnabledExtensionNames = device_extensions.Data;
                create_info.pNext = &enabledFeatures;
                err =
                    vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
                check_vk_result(err);
                vkGetDeviceQueue(g_Device, g_QueueFamily[0], 0, &g_RenderQueue[0]);
                vkGetDeviceQueue(g_Device, g_QueueFamily[0], 1, &g_RenderQueue[1]);
                vkGetDeviceQueue(g_Device, g_QueueFamily[1], 0, &g_ComputeQueue);
            }

            // Create Descriptor Pool
            {
                VkDescriptorPoolSize pool_sizes[] = {
                    {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000} };
                VkDescriptorPoolCreateInfo pool_info = {};
                pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
                pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
                pool_info.pPoolSizes = pool_sizes;
                err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator,
                    &g_DescriptorPool);
                check_vk_result(err);
            }
        }

        // All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used
        // by the demo. Your real engine/app may not use them.
        static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd,
            VkSurfaceKHR surface, int width, int height) {
            wd->Surface = surface;

            // Check for WSI support
            VkBool32 res;
            vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily[0],
                wd->Surface, &res);
            if (res != VK_TRUE) {
                fprintf(stderr, "Error no WSI support on physical device 0\n");
                exit(-1);
            }

            // Select Surface Format
            const VkFormat requestSurfaceImageFormat[] = {
                VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
            const VkColorSpaceKHR requestSurfaceColorSpace =
                VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
                g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat,
                (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
                requestSurfaceColorSpace);

            // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
            VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR,
                                                VK_PRESENT_MODE_IMMEDIATE_KHR,
                                                VK_PRESENT_MODE_FIFO_KHR };
#else
            VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
            wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
                g_PhysicalDevice, wd->Surface, &present_modes[0],
                IM_ARRAYSIZE(present_modes));
            // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

            // Create SwapChain, RenderPass, Framebuffer, etc.
            IM_ASSERT(g_MinImageCount >= 2);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device,
                wd, g_QueueFamily[0], g_Allocator, width,
                height, g_MinImageCount);
        }

        static void CleanupVulkan() {
            vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
            // Remove the debug report callback
            auto vkDestroyDebugReportCallbackEXT =
                (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                    g_Instance, "vkDestroyDebugReportCallbackEXT");
            vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

            vkDestroyDevice(g_Device, g_Allocator);
            vkDestroyInstance(g_Instance, g_Allocator);
        }

        static void CleanupVulkanWindow() {
            ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData,
                g_Allocator);
        }

        static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data) {
            VkResult err;

            VkSemaphore image_acquired_semaphore =
                wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
            VkSemaphore render_complete_semaphore =
                wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
            err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX,
                image_acquired_semaphore, VK_NULL_HANDLE,
                &wd->FrameIndex);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
                g_SwapChainRebuild = true;
                return;
            }
            check_vk_result(err);

            ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
            {
                err = vkWaitForFences(
                    g_Device, 1, &fd->Fence, VK_TRUE,
                    UINT64_MAX); // wait indefinitely instead of periodically checking
                check_vk_result(err);

                err = vkResetFences(g_Device, 1, &fd->Fence);
                check_vk_result(err);
            }
            {
                err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
                check_vk_result(err);
                VkCommandBufferBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
                check_vk_result(err);
            }
            {
                VkRenderPassBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                info.renderPass = wd->RenderPass;
                info.framebuffer = fd->Framebuffer;
                info.renderArea.extent.width = wd->Width;
                info.renderArea.extent.height = wd->Height;
                info.clearValueCount = 1;
                info.pClearValues = &wd->ClearValue;
                vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
            }

            // Record dear imgui primitives into command buffer
            ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

            // Submit command buffer
            vkCmdEndRenderPass(fd->CommandBuffer);
            {
                VkPipelineStageFlags wait_stage =
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                VkSubmitInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                info.waitSemaphoreCount = 1;
                info.pWaitSemaphores = &image_acquired_semaphore;
                info.pWaitDstStageMask = &wait_stage;
                info.commandBufferCount = 1;
                info.pCommandBuffers = &fd->CommandBuffer;
                info.signalSemaphoreCount = 1;
                info.pSignalSemaphores = &render_complete_semaphore;

                err = vkEndCommandBuffer(fd->CommandBuffer);
                check_vk_result(err);
                err = vkQueueSubmit(g_RenderQueue[0], 1, &info, fd->Fence);
                check_vk_result(err);
            }
        }

        static void FramePresent(ImGui_ImplVulkanH_Window* wd) {
            if (g_SwapChainRebuild)
                return;
            VkSemaphore render_complete_semaphore =
                wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
            VkPresentInfoKHR info = {};
            info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &render_complete_semaphore;
            info.swapchainCount = 1;
            info.pSwapchains = &wd->Swapchain;
            info.pImageIndices = &wd->FrameIndex;
            VkResult err = vkQueuePresentKHR(g_RenderQueue[0], &info);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
                g_SwapChainRebuild = true;
                return;
            }
            check_vk_result(err);
            wd->SemaphoreIndex =
                (wd->SemaphoreIndex + 1) %
                wd->ImageCount; // Now we can use the next set of semaphores
        }
#pragma endregion
	};
}