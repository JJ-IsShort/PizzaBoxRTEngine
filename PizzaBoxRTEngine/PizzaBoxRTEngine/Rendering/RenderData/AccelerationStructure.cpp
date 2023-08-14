#include "AccelerationStructure.h"
#include <vulkan/vulkan_core.h>
#include <iostream>
#include "app.h"

namespace PBEngine
{
    extern App app;

    // TODO: Make a seperate buffer class inside VulkanHelp, with this as a static function maybe?
    /*// Function to create a Vulkan buffer
    void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        // Create the buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

        // Allocate memory for the buffer
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);

        // Bind the memory to the buffer
        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    // Function to find suitable memory type for the buffer
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        // Handle failure to find suitable memory type
        // Add error handling or fallback behavior here
        return 0;
    }*/

    // Helper function to find a suitable memory type index
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

    // Function to create a Vulkan buffer and allocate memory for it
    void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
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
    void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkBufferCreateInfo& bufferCreateInfo) {
        // Set up the buffer
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

    AccelerationStructure::AccelerationStructure()
    {
        accelStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
        accelStructureInfo.pNext = NULL;
        accelStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        // Where Geometry happens
        //geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
        //geometry.pNext = NULL;
        //geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
        //geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

        // Create a VkBuffer to store the vertex data
        VkBufferCreateInfo bufferCreateInfo = {};
        createBuffer(app.g_Device, app.g_PhysicalDevice, sizeof(float) * triangleVertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory, bufferCreateInfo);

        // Fill the vertex buffer with the vertex data
        void* mappedMemory;
        vkMapMemory(app.g_Device, vertexBufferMemory, 0, bufferCreateInfo.size, 0, &mappedMemory);
        memcpy(mappedMemory, triangleVertices.data(), bufferCreateInfo.size);
        vkUnmapMemory(app.g_Device, vertexBufferMemory);

        // Get buffer device address
        VkBufferDeviceAddressInfo info = {};
        info.buffer = vertexBuffer;

        VkDeviceAddress vertexAddress = vkGetBufferDeviceAddress(app.g_Device, &info);

        // Populate geometry data struct
        geometryData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometryData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometryData.vertexData.deviceAddress = vertexAddress;
        geometryData.maxVertex = triangleVertices.size() / 3;
        geometryData.vertexStride = sizeof(float) * 3;
        geometryData.indexType = VK_INDEX_TYPE_NONE_KHR;

        vkCreateAccelerationStructureKHR(app.g_Device, &accelStructureInfo, nullptr, &accelStruct);

        std::vector<VkAccelerationStructureGeometryKHR> geometryDatas;
        geometryDatas[0].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometryDatas[0].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometryDatas[0].geometry.triangles = geometryData;

        VkAccelerationStructureBuildGeometryInfoKHR buildGeoInfo[1];
        buildGeoInfo[0] = {};
        buildGeoInfo[0].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildGeoInfo[0].type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildGeoInfo[0].mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeoInfo[0].dstAccelerationStructure = accelStruct;
        buildGeoInfo[0].geometryCount = 1;
        buildGeoInfo[0].pGeometries = geometryDatas.data();

        /*VkAccelerationStructureBuildGeometryInfoKHR geometryInfos;
        geometryInfos[0] = buildInfo;
        geometryInfos[0].pGeometries = &geometryData;

        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.geometryCount = 1;*/
    }

    AccelerationStructure::~AccelerationStructure()
    {
        vkDestroyBuffer(app.g_Device, vertexBuffer, nullptr);
        vkFreeMemory(app.g_Device, vertexBufferMemory, nullptr);
    }

    /*void AccelerationStructure::createBottomLevelAccelerationStructure(VkDevice device,
                                                                        VkPhysicalDevice physicalDevice, 
                                                                        VkCommandPool commandPool, 
                                                                        VkQueue queue, 
                                                                        const std::vector<Vertex>& vertices, 
                                                                        const std::vector<Index>& indices, 
                                                                        VkAccelerationStructureKHR& blas, 
                                                                        VkBuffer& vertexBuffer, 
                                                                        VkDeviceMemory& vertexBufferMemory, 
                                                                        VkBuffer& indexBuffer, 
                                                                        VkDeviceMemory& indexBufferMemory)
	{
        // Create and upload vertex buffer
        VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
        createBuffer(device, physicalDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertexBuffer, vertexBufferMemory);

        void* vertexData;
        vkMapMemory(device, vertexBufferMemory, 0, vertexBufferSize, 0, &vertexData);
        memcpy(vertexData, vertices.data(), static_cast<size_t>(vertexBufferSize));
        vkUnmapMemory(device, vertexBufferMemory);

        // Create and upload index buffer
        VkDeviceSize indexBufferSize = sizeof(Index) * indices.size();
        createBuffer(device, physicalDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            indexBuffer, indexBufferMemory);

        void* indexData;
        vkMapMemory(device, indexBufferMemory, 0, indexBufferSize, 0, &indexData);
        memcpy(indexData, indices.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(device, indexBufferMemory);

        // Create BLAS geometry
        VkAccelerationStructureBuildGeometryInfoKHR geometryInfo{};
        geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
        geometryInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometryInfo.maxPrimitiveCount = static_cast<uint32_t>(indices.size() / 3);
        geometryInfo.indexType = VK_INDEX_TYPE_UINT32;
        geometryInfo.maxVertexCount = static_cast<uint32_t>(vertices.size());
        geometryInfo.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometryInfo.allowsTransforms = VK_FALSE; // Set to VK_TRUE if you need transformable geometry

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        createInfo.pNext = &geometryInfo;

        vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &blas);

        // Build the BLAS
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Optional flag
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometryInfo;

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo, &buildInfo.geometryCount, &sizeInfo);

        VkBuffer scratchBuffer;
        VkDeviceMemory scratchBufferMemory;
        createBuffer(device, physicalDevice, sizeInfo.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBuffer, scratchBufferMemory);

        VkAccelerationStructureBufferCreateInfoKHR bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUFFER_CREATE_INFO_KHR;
        bufferInfo.size = sizeInfo.accelerationStructureSize;
        bufferInfo.usage = VK_ACCELERATION_STRUCTURE_BUFFER_USAGE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_ACCELERATION_STRUCTURE_BUFFER_USAGE_STORAGE_BIT_KHR;

        VkBuffer accelerationStructureBuffer;
        VkDeviceMemory accelerationStructureBufferMemory;
        createBuffer(device, physicalDevice, bufferInfo.size,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, accelerationStructureBuffer,
            accelerationStructureBufferMemory);

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Optional flag
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometryInfo;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        buildInfo.dstAccelerationStructure = blas;
        buildInfo.scratchData.deviceAddress = getBufferDeviceAddress(device, scratchBuffer);

        VkAccelerationStructureBuildRangeInfoKHR buildRange{};
        buildRange.primitiveCount = static_cast<uint32_t>(indices.size() / 3);
        buildRange.primitiveOffset = 0;
        buildRange.firstVertex = 0;
        buildRange.transformOffset = 0; // Set to non-zero if you need transformable geometry

        VkAccelerationStructureBuildRangeInfoKHR* pBuildRange = &buildRange;

        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &pBuildRange);

        // Destroy temporary resources
        vkDestroyBuffer(device, scratchBuffer, nullptr);
        vkFreeMemory(device, scratchBufferMemory, nullptr);
	}*/
}