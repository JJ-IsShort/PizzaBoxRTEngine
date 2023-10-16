#include "vk_common.h"

#include <cstdio>

extern PBEngine::App app;

namespace PBEngine
{
    void check_vk_result(VkResult err) {
        if (err == 0)
            return;
        fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
        if (err < 0)
            abort();
    }

	uint32_t GetMemoryType(uint32_t bits, VkMemoryPropertyFlags properties, VkPhysicalDevice gpu, VkBool32* memory_type_found)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((bits & 1) == 1)
			{
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					if (memory_type_found)
					{
						*memory_type_found = true;
					}
					return i;
				}
			}
			bits >>= 1;
		}

		if (memory_type_found)
		{
			*memory_type_found = false;
			return 0;
		}
		else
		{
			throw std::runtime_error("Could not find a matching memory type");
		}
	}

	App GetApp()
	{
		return app;
	}

	VkQueue GetRenderQueue()
	{
		return app.g_RenderQueue[0];
	}
	VkQueue GetRTQueue()
	{
		return app.g_RenderQueue[1];
	}
	VkQueue GetComputeQueue()
	{
		return app.g_ComputeQueue;
	}
	
	VkDevice GetDevice()
	{
		return app.g_Device;
	}

	VkPhysicalDevice GetPhysicalDevice()
	{
		return app.g_PhysicalDevice;
	}
}