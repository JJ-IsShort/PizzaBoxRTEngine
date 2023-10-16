#include "TLAS.h"
#include <iostream>

namespace PBEngine
{
    TLAS::TLAS()
    {

    }

    TLAS::~TLAS()
    {
        if (buffer)
        {
            buffer.reset();
        }
        if (handle)
        {
            vkDestroyAccelerationStructureKHR(GetDevice(), handle, nullptr);
        }
    }

    /*
        Gets the device address from a buffer that's needed in many places during the ray tracing setup
    */
    uint64_t get_buffer_device_address(VkBuffer buffer)
    {
        VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
        buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(GetDevice(), &buffer_device_address_info);
    }

    ScratchBuffer create_scratch_buffer(VkDeviceSize size)
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

    void delete_scratch_buffer(ScratchBuffer& scratch_buffer)
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

    void TLAS::BuildTLAS()
    {
        if (blasList.size() < 1)
        {
            std::cerr << "Needs more than one BLAS." << std::endl;
        }

        VkTransformMatrixKHR identityMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f };

        std::vector<VkAccelerationStructureInstanceKHR> instancesData;
        std::vector<VkAccelerationStructureGeometryKHR> geometry;
        std::vector<std::unique_ptr<Buffer>> instancesBuffers;
        for (size_t i = 0; i < GetNumInstances(); i++)
        {
            VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
            acceleration_structure_instance.transform = identityMatrix;
            acceleration_structure_instance.instanceCustomIndex = 0;
            acceleration_structure_instance.mask = 0xFF;
            acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
            acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            acceleration_structure_instance.accelerationStructureReference = blasList[i].deviceAddress;

            std::unique_ptr<Buffer> instances_buffer = std::make_unique<Buffer>(GetDevice(), GetPhysicalDevice(),
                sizeof(VkAccelerationStructureInstanceKHR),
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            instances_buffer->update(&acceleration_structure_instance, sizeof(VkAccelerationStructureInstanceKHR));

            VkDeviceOrHostAddressConstKHR instance_data_device_address{};
            instance_data_device_address.deviceAddress = get_buffer_device_address(instances_buffer->get_handle());

            // The top level acceleration structure contains (bottom level) instance as the input geometry
            VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
            acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
            acceleration_structure_geometry.geometry.instances.data = instance_data_device_address;

            instancesData.push_back(std::move(acceleration_structure_instance));
            instancesBuffers.push_back(std::move(instances_buffer));
            geometry.push_back(std::move(acceleration_structure_geometry));
        }

        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
        acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_structure_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        acceleration_structure_build_geometry_info.geometryCount = GetNumInstances();
        acceleration_structure_build_geometry_info.pGeometries = geometry.data();

        const uint32_t primitive_count = GetNumInstances();

        VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
        acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &acceleration_structure_build_geometry_info,
            &primitive_count,
            &acceleration_structure_build_sizes_info);

        // Create a buffer to hold the acceleration structure
        buffer = std::make_unique<Buffer>(
            GetDevice(), GetPhysicalDevice(),
            acceleration_structure_build_sizes_info.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Create the acceleration structure
        VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
        acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        acceleration_structure_create_info.buffer = buffer->get_handle();
        acceleration_structure_create_info.size = acceleration_structure_build_sizes_info.accelerationStructureSize;
        acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(GetDevice(), &acceleration_structure_create_info, nullptr, &handle);

        // The actual build process starts here

        // Create a scratch buffer as a temporary storage for the acceleration structure build
        ScratchBuffer scratch_buffer = create_scratch_buffer(acceleration_structure_build_sizes_info.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
        acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        acceleration_build_geometry_info.dstAccelerationStructure = handle;
        acceleration_build_geometry_info.geometryCount = GetNumInstances();
        acceleration_build_geometry_info.pGeometries = geometry.data();
        acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.device_address;

        VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
        acceleration_structure_build_range_info.primitiveCount = GetNumInstances();
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

        // Get the top acceleration structure's handle, which will be used to setup it's descriptor
        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = handle;
        deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(GetDevice(), &acceleration_device_address_info);
    }
}