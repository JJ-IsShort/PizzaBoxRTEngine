#include "Buffer.h"
#include <iostream>

namespace PBEngine
{
	Buffer::~Buffer()
	{
        unmap();
		vkDestroyBuffer(GetApp().g_Device, buffer, nullptr);
		vkFreeMemory(GetApp().g_Device, bufferMemory, nullptr);
	}

    Buffer::Buffer(Buffer&& other) :
        bufferMemory{ other.bufferMemory },
        bufferSize{ other.bufferSize },
        mapped_data{ other.mapped_data },
        mapped{ other.mapped }
    {
        // Reset other handles to avoid releasing on destruction
        other.bufferMemory = VK_NULL_HANDLE;
        other.mapped_data = nullptr;
        other.mapped = false;
    }

    // Helper function to find a suitable memory type index
    uint32_t Buffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        std::cerr << "Failed to find suitable memory type." << std::endl;
        return 0;
    }

    Buffer::Buffer(VkDevice g_Device, VkPhysicalDevice g_PhysicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        device = g_Device;
        physicalDevice = g_PhysicalDevice;
        bufferSize = size;

        // Create the buffer
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
            std::cerr << "Failed to create Vulkan buffer." << std::endl;
            return;
        }

        // Allocate memory for the buffer
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        VkMemoryAllocateFlagsInfo allocateFlags = {};
        VkMemoryAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = &allocateFlags;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);

        allocateFlags.pNext = nullptr;
        allocateFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;

        if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            allocateFlags.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        }

        if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            std::cerr << "Failed to allocate memory for Vulkan buffer." << std::endl;
            return;
        }

        // Bind the memory to the buffer
        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

	Buffer::Buffer(VkDevice g_Device, VkPhysicalDevice g_PhysicalDevice, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBufferCreateFlags& bufferCreateFlags)
	{
        device = g_Device;
        physicalDevice = g_PhysicalDevice;
        bufferSize = size;

        // Set up the buffer
        VkBufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.flags = bufferCreateFlags;

        if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
            std::cerr << "Failed to create Vulkan buffer." << std::endl;
            return;
        }

        // Allocate memory for the buffer
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            std::cerr << "Failed to allocate memory for Vulkan buffer." << std::endl;
            return;
        }

        // Bind the memory to the buffer
        vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

    const VkBuffer* Buffer::get() const
    {
        return &buffer;
    }

    VkBuffer Buffer::get_handle()
    {
        return buffer;
    }

    VkDeviceSize Buffer::get_size()
    {
        return bufferSize;
    }

    VkDeviceMemory Buffer::get_memory() const
    {
        return bufferMemory;
    }

    void* Buffer::map()
    {
        if (!mapped && !mapped_data)
        {
            VkResult err;
            void* mappedMemory;
            vkMapMemory(GetApp().g_Device, bufferMemory, 0, bufferSize, 0, &mapped_data);
            
            mapped = true;
        }
        return mapped_data;
    }

    void Buffer::unmap()
    {
        if (mapped)
        {
            vkUnmapMemory(GetApp().g_Device, bufferMemory);
            mapped_data = nullptr;
            mapped = false;
        }
    }

    const void* Buffer::get_data() const
    {
        return mapped_data;
    }

    void Buffer::update(const std::vector<uint8_t>& data, size_t offset)
    {
        update(data.data(), data.size(), offset);
    }

    void Buffer::update(void* data, size_t size, size_t offset)
    {
        update(reinterpret_cast<const uint8_t*>(data), size, offset);
    }

    void Buffer::update(const uint8_t* data, const size_t size, const size_t offset)
    {
        map();
        memcpy(mapped_data, data, bufferSize);
        unmap();
    }

    template <class T>
    void Buffer::convert_and_update(const T& object, size_t offset)
    {
        update(reinterpret_cast<const uint8_t*>(&object), sizeof(T), offset);
    }

    uint64_t Buffer::get_device_address()
    {
        VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
        buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(device, &buffer_device_address_info);
    }
}