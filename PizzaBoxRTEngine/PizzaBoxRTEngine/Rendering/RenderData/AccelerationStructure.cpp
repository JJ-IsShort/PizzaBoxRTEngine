#include "AccelerationStructure.h"
#include <vulkan/vulkan_core.h>
#include <iostream>
#include <memory>
#include <VulkanHelp/Buffer.h>
#include "app.h"

namespace PBEngine
{
    AccelerationStructure::AccelerationStructure(AccelerationStructure&& other) :
        handle(other.handle),
        deviceAddress(other.deviceAddress),
        buffer(std::move(other.buffer)),
        vertexBuffer(std::move(other.vertexBuffer)),
        indexBuffer(std::move(other.indexBuffer)),
        indexCount(other.indexCount)
    {
        // Leave other in valid empty state
        other.handle = VK_NULL_HANDLE;
        other.deviceAddress = 0;
        other.buffer = nullptr;
        other.vertexBuffer = nullptr;
        other.indexBuffer = nullptr;
        other.indexCount = 0;
    }

    /*
        Gets the device address from a buffer that's needed in many places during the ray tracing setup
    */
    uint64_t AccelerationStructure::get_buffer_device_address(VkBuffer buffer)
    {
        VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
        buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(GetDevice(), &buffer_device_address_info);
    }

    ScratchBuffer AccelerationStructure::create_scratch_buffer(VkDeviceSize size)
    {
        ScratchBuffer scratch_buffer{};

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.size = size;
        buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        check_vk_result(vkCreateBuffer(GetDevice(), &buffer_create_info, nullptr, &scratch_buffer.handle));

        VkMemoryRequirements memory_requirements = {};
        vkGetBufferMemoryRequirements(GetDevice(), scratch_buffer.handle, &memory_requirements);

        VkMemoryAllocateFlagsInfo memory_allocate_flags_info = {};
        memory_allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        memory_allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

        VkBool32 memTypeFound = false;
        VkMemoryAllocateInfo memory_allocate_info = {};
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.pNext = &memory_allocate_flags_info;
        memory_allocate_info.allocationSize = memory_requirements.size;
        memory_allocate_info.memoryTypeIndex = GetMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, GetPhysicalDevice(), &memTypeFound);
        check_vk_result(vkAllocateMemory(GetDevice(), &memory_allocate_info, nullptr, &scratch_buffer.memory));
        check_vk_result(vkBindBufferMemory(GetDevice(), scratch_buffer.handle, scratch_buffer.memory, 0));

        VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
        buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer = scratch_buffer.handle;
        scratch_buffer.device_address = vkGetBufferDeviceAddressKHR(GetDevice(), &buffer_device_address_info);

        return scratch_buffer;
    }

    void AccelerationStructure::delete_scratch_buffer(ScratchBuffer& scratch_buffer)
    {
        if (scratch_buffer.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(GetDevice(), scratch_buffer.memory, nullptr);
        }
        if (scratch_buffer.handle != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(GetDevice(), scratch_buffer.handle, nullptr);
        }
    }

    AccelerationStructure::AccelerationStructure(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices)
    {
        Initialize(vertices, indices);
    }

    AccelerationStructure::AccelerationStructure()
    {
        // Define the vertex data for the triangle
        std::vector<glm::vec3> vertices;
        vertices.push_back({ -0.5f, -0.5f, 0.0f });
        vertices.push_back({ 0.5f, -0.5f, 0.0f });
        vertices.push_back({ 0.0f,  0.5f, 0.0f });
        std::vector<uint32_t> indices = { 0, 1, 2 };
        Initialize(vertices, indices);
    }

    void AccelerationStructure::Initialize(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices)
    {
        size_t vertex_buffer_size = vertices.size() * sizeof(glm::vec3);
        size_t index_buffer_size = indices.size() * sizeof(uint32_t);

        // Create buffers for the bottom level geometry
        const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VkMemoryPropertyFlags bufferMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        void* triangleVoid = const_cast<void*>(reinterpret_cast<const void*>(vertices.data()));
        vertexBuffer = std::make_unique<Buffer>(GetDevice(), GetPhysicalDevice(), vertex_buffer_size, bufferUsageFlags, bufferMemoryFlags);
        vertexBuffer->update(triangleVoid, vertex_buffer_size);

        void* indexVoid = const_cast<void*>(reinterpret_cast<const void*>(indices.data()));
        indexBuffer = std::make_unique<Buffer>(GetDevice(), GetPhysicalDevice(), index_buffer_size, bufferUsageFlags, bufferMemoryFlags);
        indexBuffer->update(indexVoid, index_buffer_size);

        // Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
        VkTransformMatrixKHR transform_matrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f };
        std::unique_ptr<Buffer> transform_matrix_buffer = std::make_unique<Buffer>(GetDevice(), GetPhysicalDevice(), sizeof(transform_matrix), bufferUsageFlags, bufferMemoryFlags);
        transform_matrix_buffer->update(&transform_matrix, sizeof(transform_matrix));

        VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
        VkDeviceOrHostAddressConstKHR index_data_device_address{};
        VkDeviceOrHostAddressConstKHR transform_matrix_device_address{};

        vertex_data_device_address.deviceAddress = get_buffer_device_address(vertexBuffer->get_handle());
        index_data_device_address.deviceAddress = get_buffer_device_address(indexBuffer->get_handle());
        transform_matrix_device_address.deviceAddress = get_buffer_device_address(transform_matrix_buffer->get_handle());

        VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
        acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        acceleration_structure_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        acceleration_structure_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        acceleration_structure_geometry.geometry.triangles.vertexData = vertex_data_device_address;
        acceleration_structure_geometry.geometry.triangles.maxVertex = 3;
        acceleration_structure_geometry.geometry.triangles.vertexStride = sizeof(glm::vec3);
        acceleration_structure_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        acceleration_structure_geometry.geometry.triangles.indexData = index_data_device_address;
        acceleration_structure_geometry.geometry.triangles.transformData = transform_matrix_device_address;

        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
        acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_structure_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        acceleration_structure_build_geometry_info.geometryCount = 1;
        acceleration_structure_build_geometry_info.pGeometries = &acceleration_structure_geometry;

        const uint32_t primitive_count = 1;

        VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
        acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            GetDevice(),
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &acceleration_structure_build_geometry_info,
            &primitive_count,
            &acceleration_structure_build_sizes_info);

        // Create a buffer to hold the acceleration structure
        buffer = std::make_unique<Buffer>(
            GetDevice(),
            GetPhysicalDevice(),
            acceleration_structure_build_sizes_info.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            0);

        // Create the acceleration structure
        VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
        acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        acceleration_structure_create_info.buffer = buffer->get_handle();
        acceleration_structure_create_info.size = acceleration_structure_build_sizes_info.accelerationStructureSize;
        acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(GetDevice(), &acceleration_structure_create_info, nullptr, &handle);

        // The actual build process starts here

        // Create a scratch buffer as a temporary storage for the acceleration structure build
        ScratchBuffer scratch_buffer = create_scratch_buffer(acceleration_structure_build_sizes_info.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
        acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        acceleration_build_geometry_info.dstAccelerationStructure = handle;
        acceleration_build_geometry_info.geometryCount = 1;
        acceleration_build_geometry_info.pGeometries = &acceleration_structure_geometry;
        acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.device_address;

        VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
        acceleration_structure_build_range_info.primitiveCount = 1;
        acceleration_structure_build_range_info.primitiveOffset = 0;
        acceleration_structure_build_range_info.firstVertex = 0;
        acceleration_structure_build_range_info.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = { &acceleration_structure_build_range_info };

        // Build the acceleration structure on the device via a one-time command buffer submission
        // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
        // Create the command pool
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = GetApp().g_QueueFamily[1];
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        check_vk_result(vkCreateCommandPool(GetDevice(), &poolCreateInfo, nullptr, &commandPool));

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Primary command buffer
        commandBufferAllocateInfo.commandBufferCount = 1; // Allocate a single command buffer

        check_vk_result(vkAllocateCommandBuffers(GetDevice(), &commandBufferAllocateInfo, &commandBuffer));

        VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);

        vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &acceleration_build_geometry_info,
            acceleration_build_structure_range_infos.data());

        // Flush command buffer
        check_vk_result(vkEndCommandBuffer(commandBuffer));
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &commandBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = 0;

        VkFence fence;
        check_vk_result(vkCreateFence(GetDevice(), &fence_info, nullptr, &fence));

        // Submit to the queue
        VkResult result = vkQueueSubmit(GetComputeQueue(), 1, &submit_info, fence);
        // Wait for the fence to signal that command buffer has finished executing
        check_vk_result(vkWaitForFences(GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX));

        vkDestroyFence(GetDevice(), fence, nullptr);
        vkFreeCommandBuffers(GetDevice(), commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(GetDevice(), commandPool, nullptr);

        delete_scratch_buffer(scratch_buffer);

        // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = handle;
        deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(GetDevice(), &acceleration_device_address_info);
    }

    AccelerationStructure::~AccelerationStructure()
    {
        if (buffer)
        {
            buffer.reset();
        }
        if (handle)
        {
            vkDestroyAccelerationStructureKHR(GetDevice(), handle, nullptr);
        }
        vertexBuffer.reset();
        indexBuffer.reset();
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