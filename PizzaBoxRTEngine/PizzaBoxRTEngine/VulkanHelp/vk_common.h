#pragma once

#include "../../../External/volk/volk.h"
#include "app.h"

namespace PBEngine
{
	void check_vk_result(VkResult err);

	/*uint32_t GetMemoryType(uint32_t bits, VkMemoryPropertyFlags properties, VkPhysicalDevice gpu)
	{
		VkBool32 temp = false;
		return GetMemoryType(bits, properties, gpu, &temp);
	}*/

	uint32_t GetMemoryType(uint32_t bits, VkMemoryPropertyFlags properties, VkPhysicalDevice gpu, VkBool32* memory_type_found);

	App GetApp();

	// Get Queues
	VkQueue GetRenderQueue();
	VkQueue GetRTQueue();
	VkQueue GetComputeQueue();

	VkDevice GetDevice();
	
	VkPhysicalDevice GetPhysicalDevice();
}