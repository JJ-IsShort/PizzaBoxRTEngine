#include "Renderer.h"

#include <stdio.h>
#include <VulkanHelp/GLSLCompiler.h>

namespace PBEngine
{
#pragma region Renderer definitions
    Renderer::Renderer(float *width, float *height)
    {
        renderingBackend = std::make_unique<Backend_FullRT>();
        if (!renderingBackend->Init(width, height)) {
            fprintf(stderr, "Trouble loading rendering backend of type: %d",
                renderingBackend->backendType);
        }
    }

    Renderer::Renderer()
    {
        renderingBackend = nullptr;
    }

    Renderer::~Renderer()
    {
        
    }
#pragma endregion

#pragma region General backend definitions
    Backend::~Backend() {};
    bool Backend::Init(float *width, float *height) { return false; }
    bool Backend::Render() { return false; }
    bool Backend::CleanupBackend() { return false; }
#pragma endregion

#pragma region FullRT backend definitions
    Backend_FullRT::~Backend_FullRT()
    {
        CleanupBackend();
    }

    void Backend_FullRT::CreateViewImage(bool resize = false)
    {
        viewImage.width = static_cast<uint32_t>(truncf(*viewportWidth));
        viewImage.height = static_cast<uint32_t>(truncf(*viewportHeight));

        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.flags = 0;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = VK_FORMAT_B8G8R8A8_UNORM;
        image.extent.width = viewImage.width;
        image.extent.height = viewImage.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        check_vk_result(vkCreateImage(GetDevice(), &image, nullptr, &viewImage.image));

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(GetDevice(), viewImage.image, &memory_requirements);
        VkMemoryAllocateInfo memory_allocate_info{};
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.allocationSize = memory_requirements.size;
        VkBool32 memFound = false;
        memory_allocate_info.memoryTypeIndex = GetMemoryType(memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, GetPhysicalDevice(), &memFound);
        check_vk_result(vkAllocateMemory(GetDevice(), &memory_allocate_info, nullptr, &viewImage.memory));
        check_vk_result(vkBindImageMemory(GetDevice(), viewImage.image, viewImage.memory, 0));

        if (!resize)
        {
            VkImageViewCreateInfo color_image_view{};
            color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            color_image_view.format = VK_FORMAT_B8G8R8A8_UNORM;
            color_image_view.subresourceRange = {};
            color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color_image_view.subresourceRange.baseMipLevel = 0;
            color_image_view.subresourceRange.levelCount = 1;
            color_image_view.subresourceRange.baseArrayLayer = 0;
            color_image_view.subresourceRange.layerCount = 1;
            color_image_view.image = viewImage.image;
            check_vk_result(vkCreateImageView(GetDevice(), &color_image_view, nullptr, &viewImage.view));
        }

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // outside image bounds just use border color
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.minLod = -1000;
        sampler_info.maxLod = 1000;
        sampler_info.maxAnisotropy = 1.0f;
        vkCreateSampler(GetDevice(), &sampler_info, nullptr, &viewImage.sampler);

        VkCommandPool imageCmdPool;
        // Create the command pool
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = GetApp().g_QueueFamily[1];
        poolCreateInfo.flags = 0; // Optional flags, such as VK_COMMAND_POOL_CREATE_TRANSIENT_BIT or VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

        VkResult result = vkCreateCommandPool(GetDevice(), &poolCreateInfo, nullptr, &imageCmdPool);
        if (result != VK_SUCCESS) {
            // Handle command pool creation failure
            fprintf(stdout, "Couldn't create Command Pool: %d\n", result);
        }

        VkCommandBufferAllocateInfo cmd_buf_allocate_info{};
        cmd_buf_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_allocate_info.commandPool = imageCmdPool;
        cmd_buf_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buf_allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        check_vk_result(vkAllocateCommandBuffers(GetDevice(), &cmd_buf_allocate_info, &command_buffer));

        VkCommandBufferBeginInfo command_buffer_info{};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        check_vk_result(vkBeginCommandBuffer(command_buffer, &command_buffer_info));

        // Create an image barrier object
        VkImageMemoryBarrier image_memory_barrier{};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcAccessMask = {};
        image_memory_barrier.dstAccessMask = {};
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = viewImage.image;
        image_memory_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

        check_vk_result(vkEndCommandBuffer(command_buffer));
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = 0;

        VkFence fence;
        check_vk_result(vkCreateFence(GetDevice(), &fence_info, nullptr, &fence));

        // Submit to the queue
        result = vkQueueSubmit(GetComputeQueue(), 1, &submit_info, fence);
        // Wait for the fence to signal that command buffer has finished executing
        check_vk_result(vkWaitForFences(GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX));

        vkDestroyFence(GetDevice(), fence, nullptr);
        vkFreeCommandBuffers(GetDevice(), imageCmdPool, 1, &command_buffer);
        vkDestroyCommandPool(GetDevice(), imageCmdPool, nullptr);
    }

    void Backend_FullRT::CreateStorageImage()
    {
        storage_image.width = static_cast<uint32_t>(truncf(*viewportWidth));
        storage_image.height = static_cast<uint32_t>(truncf(*viewportHeight));

        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.flags = 0;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = VK_FORMAT_B8G8R8A8_UNORM;
        image.extent.width = storage_image.width;
        image.extent.height = storage_image.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        check_vk_result(vkCreateImage(GetDevice(), &image, nullptr, &storage_image.image));

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(GetDevice(), storage_image.image, &memory_requirements);
        VkMemoryAllocateInfo memory_allocate_info{};
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.allocationSize = memory_requirements.size;
        VkBool32 memFound = false;
        memory_allocate_info.memoryTypeIndex = GetMemoryType(memory_requirements.memoryTypeBits, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, GetPhysicalDevice(), &memFound);
        check_vk_result(vkAllocateMemory(GetDevice(), &memory_allocate_info, nullptr, &storage_image.memory));
        check_vk_result(vkBindImageMemory(GetDevice(), storage_image.image, storage_image.memory, 0));

        VkImageViewCreateInfo color_image_view{};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.format = VK_FORMAT_B8G8R8A8_UNORM;
        color_image_view.subresourceRange = {};
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.levelCount = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.layerCount = 1;
        color_image_view.image = storage_image.image;
        check_vk_result(vkCreateImageView(GetDevice(), &color_image_view, nullptr, &storage_image.view));

        VkCommandPool imageCmdPool;
        // Create the command pool
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = GetApp().g_QueueFamily[1];
        poolCreateInfo.flags = 0; // Optional flags, such as VK_COMMAND_POOL_CREATE_TRANSIENT_BIT or VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

        VkResult result = vkCreateCommandPool(GetDevice(), &poolCreateInfo, nullptr, &imageCmdPool);
        if (result != VK_SUCCESS) {
            // Handle command pool creation failure
            fprintf(stdout, "Couldn't create Command Pool: %d\n", result);
        }

        VkCommandBufferAllocateInfo cmd_buf_allocate_info{};
        cmd_buf_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_allocate_info.commandPool = imageCmdPool;
        cmd_buf_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buf_allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        check_vk_result(vkAllocateCommandBuffers(GetDevice(), &cmd_buf_allocate_info, &command_buffer));

        VkCommandBufferBeginInfo command_buffer_info{};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        check_vk_result(vkBeginCommandBuffer(command_buffer, &command_buffer_info));

        // Create an image barrier object
        VkImageMemoryBarrier image_memory_barrier{};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcAccessMask = {};
        image_memory_barrier.dstAccessMask = {};
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = storage_image.image;
        image_memory_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
            0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

        check_vk_result(vkEndCommandBuffer(command_buffer));
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = 0;

        VkFence fence;
        check_vk_result(vkCreateFence(GetDevice(), &fence_info, nullptr, &fence));

        // Submit to the queue
        result = vkQueueSubmit(GetComputeQueue(), 1, &submit_info, fence);
        // Wait for the fence to signal that command buffer has finished executing
        check_vk_result(vkWaitForFences(GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX));

        vkDestroyFence(GetDevice(), fence, nullptr);
        vkFreeCommandBuffers(GetDevice(), imageCmdPool, 1, &command_buffer);
        vkDestroyCommandPool(GetDevice(), imageCmdPool, nullptr);
    }

    /*
        Create our ray tracing pipeline
    */
    void Backend_FullRT::CreateRayTracingPipeline()
    {
        // Slot for binding top level acceleration structures to the ray generation shader
        VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
        acceleration_structure_layout_binding.binding = 0;
        acceleration_structure_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        acceleration_structure_layout_binding.descriptorCount = 1;
        acceleration_structure_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding result_image_layout_binding{};
        result_image_layout_binding.binding = 1;
        result_image_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        result_image_layout_binding.descriptorCount = 1;
        result_image_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        /*VkDescriptorSetLayoutBinding uniform_buffer_binding{};
        uniform_buffer_binding.binding = 2;
        uniform_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_buffer_binding.descriptorCount = 1;
        uniform_buffer_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;*/

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            acceleration_structure_layout_binding,
            result_image_layout_binding };

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();
        check_vk_result(vkCreateDescriptorSetLayout(GetDevice(), &layout_info, nullptr, &descriptor_set_layout));

        VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;

        check_vk_result(vkCreatePipelineLayout(GetDevice(), &pipeline_layout_create_info, nullptr, &pipeline_layout));

        /*
            Setup ray tracing shader groups
            Each shader group points at the corresponding shader in the pipeline
        */
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

        // Ray generation group
        {
            const char* source = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D image;

layout(location = 0) rayPayloadEXT vec4 hitValue;

void main() 
{
	const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
	const vec2 inUV = pixelCenter/vec2(gl_LaunchSizeEXT.xy);
	vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = vec4(d.x, -d.y, -2.438, 1);
	vec4 direction = vec4(0, 0, 1, 0);

	float tmin = 0.001;
	float tmax = 10000.0;

    hitValue = vec4(0.0);

    traceRayEXT(topLevelAS, // Top level acceleraion structure
        gl_RayFlagsOpaqueEXT, // No flags
        0xff, // Instance mask
        0, // Ray type
        0, // Number of ray types
        0, // Miss shader index
        origin.xyz, // Ray origin
        tmin, // Minimum t value
        direction.xyz, // Direction
        tmax, // Minimum t value
        0); // Payload location

	imageStore(image, ivec2(gl_LaunchIDEXT.xy), hitValue);
})";
            VkPipelineShaderStageCreateInfo shaderStage = GLSLCompiler::load_shader(source, VK_SHADER_STAGE_RAYGEN_BIT_KHR, false);
            shader_stages.push_back(std::move(shaderStage));
            shaderModules.push_back(shaderStage.module);
            VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{};
            raygen_group_ci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            raygen_group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            raygen_group_ci.generalShader = static_cast<uint32_t>(shader_stages.size()) - 1;
            raygen_group_ci.closestHitShader = VK_SHADER_UNUSED_KHR;
            raygen_group_ci.anyHitShader = VK_SHADER_UNUSED_KHR;
            raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
            shader_groups.push_back(raygen_group_ci);
        }

        // Ray miss group
        {
            const char* source = R"(
#version 460 core
#extension GL_EXT_ray_tracing : enable
layout(location = 0) rayPayloadInEXT vec4 payload;

void main() {
    payload = vec4(51./255, 51./255, 51./255, 1.0);
})";
            VkPipelineShaderStageCreateInfo shaderStage = GLSLCompiler::load_shader(source, VK_SHADER_STAGE_MISS_BIT_KHR, false);
            shader_stages.push_back(std::move(shaderStage));
            shaderModules.push_back(shaderStage.module);
            VkRayTracingShaderGroupCreateInfoKHR miss_group_ci{};
            miss_group_ci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            miss_group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            miss_group_ci.generalShader = static_cast<uint32_t>(shader_stages.size()) - 1;
            miss_group_ci.closestHitShader = VK_SHADER_UNUSED_KHR;
            miss_group_ci.anyHitShader = VK_SHADER_UNUSED_KHR;
            miss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
            shader_groups.push_back(miss_group_ci);
        }

        // Ray closest hit group
        {
            const char* source = R"(
#version 460 core
#extension GL_EXT_ray_tracing : enable
layout(location = 0) rayPayloadInEXT vec4 payload;

void main() {
    payload = vec4(0.0, 1.0, 0.0, 1.0);
})";
            VkPipelineShaderStageCreateInfo shaderStage = GLSLCompiler::load_shader(source, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, false);
            shader_stages.push_back(std::move(shaderStage));
            shaderModules.push_back(shaderStage.module);
            VkRayTracingShaderGroupCreateInfoKHR closes_hit_group_ci{};
            closes_hit_group_ci.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            closes_hit_group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            closes_hit_group_ci.generalShader = VK_SHADER_UNUSED_KHR;
            closes_hit_group_ci.closestHitShader = static_cast<uint32_t>(shader_stages.size()) - 1;
            closes_hit_group_ci.anyHitShader = VK_SHADER_UNUSED_KHR;
            closes_hit_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
            shader_groups.push_back(closes_hit_group_ci);
        }

        /*
            Create the ray tracing pipeline
        */
        VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{};
        raytracing_pipeline_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        raytracing_pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
        raytracing_pipeline_create_info.pStages = shader_stages.data();
        raytracing_pipeline_create_info.groupCount = static_cast<uint32_t>(shader_groups.size());
        raytracing_pipeline_create_info.pGroups = shader_groups.data();
        raytracing_pipeline_create_info.maxPipelineRayRecursionDepth = 1;
        raytracing_pipeline_create_info.layout = pipeline_layout;
        check_vk_result(vkCreateRayTracingPipelinesKHR(GetDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
            &raytracing_pipeline_create_info, nullptr, &pipeline));
    }

    inline uint32_t aligned_size(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    void Backend_FullRT::CreateShaderBindingTables()
    {
        const uint32_t           handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;
        const uint32_t           handle_size_aligned = aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);
        const uint32_t           handle_alignment = ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
        const uint32_t           group_count = static_cast<uint32_t>(shader_groups.size());
        const uint32_t           sbt_size = group_count * handle_size_aligned;
        const VkBufferUsageFlags sbt_buffer_usage_flags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        VkBufferCreateFlags bufferCreateFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        // Raygen
        // Create binding table buffers for each shader type
        raygen_shader_binding_table = std::make_unique<Buffer>(GetDevice(), GetPhysicalDevice(), handle_size, sbt_buffer_usage_flags,
            bufferCreateFlags);
        miss_shader_binding_table = std::make_unique<Buffer>(GetDevice(), GetPhysicalDevice(), handle_size, sbt_buffer_usage_flags,
            bufferCreateFlags);
        hit_shader_binding_table = std::make_unique<Buffer>(GetDevice(), GetPhysicalDevice(), handle_size, sbt_buffer_usage_flags,
            bufferCreateFlags);

        // Copy the pipeline's shader handles into a host buffer
        std::vector<uint8_t> shader_handle_storage(sbt_size);
        check_vk_result(vkGetRayTracingShaderGroupHandlesKHR(GetDevice(), pipeline, 0, group_count, sbt_size, shader_handle_storage.data()));

        // Copy the shader handles from the host buffer to the binding tables
        uint8_t* data = static_cast<uint8_t*>(raygen_shader_binding_table->map());
        memcpy(data, shader_handle_storage.data(), handle_size);
        data = static_cast<uint8_t*>(miss_shader_binding_table->map());
        memcpy(data, shader_handle_storage.data() + handle_size_aligned, handle_size);
        data = static_cast<uint8_t*>(hit_shader_binding_table->map());
        memcpy(data, shader_handle_storage.data() + handle_size_aligned * 2, handle_size);
        raygen_shader_binding_table->unmap();
        miss_shader_binding_table->unmap();
        hit_shader_binding_table->unmap();
    }

    void Backend_FullRT::CreateDescriptorSets()
    {
        std::vector<VkDescriptorPoolSize> pool_sizes = {
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1} };// ,
            //{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1} };
        VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
        descriptor_pool_create_info.maxSets = 1;
        descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        check_vk_result(vkCreateDescriptorPool(GetDevice(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.descriptorPool = descriptor_pool;
        descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        check_vk_result(vkAllocateDescriptorSets(GetDevice(), &descriptor_set_allocate_info, &descriptor_set));

        // Setup the descriptor for binding our top level acceleration structure to the ray tracing shaders
        VkAccelerationStructureKHR sceneHandle = (*scene).GetHandle();
        VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
        descriptor_acceleration_structure_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptor_acceleration_structure_info.accelerationStructureCount = 1;
        descriptor_acceleration_structure_info.pAccelerationStructures = &sceneHandle;

        VkWriteDescriptorSet acceleration_structure_write{};
        acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        acceleration_structure_write.dstSet = descriptor_set;
        acceleration_structure_write.dstBinding = 0;
        acceleration_structure_write.descriptorCount = 1;
        acceleration_structure_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        // The acceleration structure descriptor has to be chained via pNext
        acceleration_structure_write.pNext = &descriptor_acceleration_structure_info;

        VkDescriptorImageInfo image_descriptor{};
        image_descriptor.imageView = storage_image.view;
        image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        /*VkDescriptorBufferInfo buffer_descriptor{};
        buffer_descriptor.buffer = (*ubo).get_handle();
        buffer_descriptor.range = (*ubo).get_size();*/

        VkWriteDescriptorSet result_image_write{};
        result_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        result_image_write.dstSet = descriptor_set;
        result_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        result_image_write.dstBinding = 1;
        result_image_write.pImageInfo = &image_descriptor;
        result_image_write.descriptorCount = 1;

        /*VkWriteDescriptorSet uniform_buffer_write{};
        uniform_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniform_buffer_write.dstSet = descriptor_set;
        uniform_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_buffer_write.dstBinding = 2;
        uniform_buffer_write.pBufferInfo = &buffer_descriptor;
        uniform_buffer_write.descriptorCount = 1;*/

        std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
            acceleration_structure_write,
            result_image_write };//,
            //uniform_buffer_write };
        vkUpdateDescriptorSets(GetDevice(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
    }

    void Backend_FullRT::CreateCommandPool()
    {
        VkCommandPoolCreateInfo command_pool_info = {};
        command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_info.queueFamilyIndex = GetApp().g_QueueFamily[0];
        check_vk_result(vkCreateCommandPool(GetDevice(), &command_pool_info, nullptr, &cmd_pool));
    }

    /*
        Gets the device address from a buffer that's needed in many places during the ray tracing setup
    */
    uint64_t GetBufferDeviceAddress_Renderer(VkBuffer buffer)
    {
        VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
        buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(GetDevice(), &buffer_device_address_info);
    }

    VkPipelineStageFlags getPipelineStageFlags(VkImageLayout layout)
    {
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_GENERAL:
            assert(false && "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
            return 0;
        default:
            assert(false);
            return 0;
        }
    }

    VkAccessFlags getAccessFlags(VkImageLayout layout)
    {
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return 0;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_GENERAL:
            assert(false && "Don't know how to get a meaningful VkAccessFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
            return 0;
        default:
            assert(false);
            return 0;
        }
    }

    void Backend_FullRT::BuildCommandBuffers()
    {
        /*if (static_cast<uint32_t>(truncf(*viewportWidth)) != storage_image.width || static_cast<uint32_t>(truncf(*viewportHeight)) != storage_image.height)
        {
            // If the view port size has changed, we need to recreate the storage image
            vkDestroyImageView(GetDevice(), storage_image.view, nullptr);
            vkDestroyImage(GetDevice(), storage_image.image, nullptr);
            vkFreeMemory(GetDevice(), storage_image.memory, nullptr);
            CreateStorageImage();
            // The view image too
            vkDestroyImageView(GetDevice(), storage_image.view, nullptr);
            vkDestroyImage(GetDevice(), storage_image.image, nullptr);
            vkFreeMemory(GetDevice(), storage_image.memory, nullptr);
            CreateViewImage();
            // The descriptor also needs to be updated to reference the new image
            VkDescriptorImageInfo image_descriptor{};
            image_descriptor.imageView = storage_image.view;
            image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            VkWriteDescriptorSet result_image_write{};
            result_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            result_image_write.dstSet = descriptor_set;
            result_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            result_image_write.dstBinding = 1;
            result_image_write.pImageInfo = &image_descriptor;
            result_image_write.descriptorCount = 1;
            vkUpdateDescriptorSets(GetDevice(), 1, &result_image_write, 0, VK_NULL_HANDLE);
        }*/

        uint32_t imagesInFlight = 1;

        // Create one command buffer for each swap chain image and reuse for rendering
        draw_cmd_buffers.resize(imagesInFlight);
        drawBuffersFences.resize(imagesInFlight);

        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = cmd_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = static_cast<uint32_t>(draw_cmd_buffers.size());

        check_vk_result(vkAllocateCommandBuffers(GetDevice(), &allocate_info, draw_cmd_buffers.data()));

        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
        {
            check_vk_result(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

            /*
                Setup the strided device address regions pointing at the shader identifiers in the shader binding table
            */

            const uint32_t handle_size_aligned = aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);

            VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
            raygen_shader_sbt_entry.deviceAddress = GetBufferDeviceAddress_Renderer(raygen_shader_binding_table->get_handle());
            raygen_shader_sbt_entry.stride = handle_size_aligned;
            raygen_shader_sbt_entry.size = handle_size_aligned;

            VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
            miss_shader_sbt_entry.deviceAddress = GetBufferDeviceAddress_Renderer(miss_shader_binding_table->get_handle());
            miss_shader_sbt_entry.stride = handle_size_aligned;
            miss_shader_sbt_entry.size = handle_size_aligned;

            VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
            hit_shader_sbt_entry.deviceAddress = GetBufferDeviceAddress_Renderer(hit_shader_binding_table->get_handle());
            hit_shader_sbt_entry.stride = handle_size_aligned;
            hit_shader_sbt_entry.size = handle_size_aligned;

            VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

            /*VkDebugUtilsLabelEXT debuggerMessage{};
            debuggerMessage.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            debuggerMessage.pLabelName = "RT Rendering \"Swapchain\" Image";
            vkCmdInsertDebugUtilsLabelEXT(draw_cmd_buffers[i], &debuggerMessage);*/

            /*
                Dispatch the ray tracing commands
            */
            {
                std::vector<VkDescriptorSet> descSetArray;
                descSetArray.push_back(descriptor_set);
                vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
                vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_layout, 0, 1, descSetArray.data(), 0, 0);
            }

            vkCmdTraceRaysKHR(
                draw_cmd_buffers[i],
                &raygen_shader_sbt_entry,
                &miss_shader_sbt_entry,
                &hit_shader_sbt_entry,
                &callable_shader_sbt_entry,
                static_cast<uint32_t>(truncf(*viewportWidth)),
                static_cast<uint32_t>(truncf(*viewportHeight)),
                1);

            auto FormatTransfer = [=](VkImage image, VkImageLayout src = VK_IMAGE_LAYOUT_UNDEFINED,
                VkImageLayout dst = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.srcAccessMask = {};
                barrier.dstAccessMask = {};
                barrier.oldLayout = src;
                barrier.newLayout = dst;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = image;
                barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

                vkCmdPipelineBarrier(draw_cmd_buffers[i],
                    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &barrier);
            };

            FormatTransfer(storage_image.image);
            FormatTransfer(viewImage.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkImageCopy copyRegion{};

            // Image extent 
            copyRegion.extent.width = static_cast<uint32_t>(truncf(*viewportWidth));
            copyRegion.extent.height = static_cast<uint32_t>(truncf(*viewportHeight));
            copyRegion.extent.depth = 1;

            // Aspect mask, typically COLOR for an RGB image
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.layerCount = 1;

            vkCmdCopyImage(draw_cmd_buffers[i],
                storage_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                viewImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &copyRegion);

            FormatTransfer(storage_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            FormatTransfer(viewImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            VkFenceCreateInfo fenceCreateInfo{};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = i == 0 ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
            vkCreateFence(GetDevice(), &fenceCreateInfo, nullptr, &drawBuffersFences[i]);

            check_vk_result(vkEndCommandBuffer(draw_cmd_buffers[i]));
        }
    }

    void Backend_FullRT::DestroyDrawBuffers()
    {
        vkDestroyFence(GetDevice(), drawBuffersFences[0], nullptr);
        vkFreeCommandBuffers(GetDevice(), cmd_pool, static_cast<uint32_t>(draw_cmd_buffers.size()), draw_cmd_buffers.data());
    }

    bool Backend_FullRT::Init(float *width, float *height) {
        // Get the ray tracing pipeline properties, which we'll need later on in the sample
        ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 device_properties{};
        device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        device_properties.pNext = &ray_tracing_pipeline_properties;
        vkGetPhysicalDeviceProperties2(GetPhysicalDevice(), &device_properties);

        // Get the acceleration structure features, which we'll need later on in the sample
        acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 device_features{};
        device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        device_features.pNext = &acceleration_structure_features;
        vkGetPhysicalDeviceFeatures2(GetPhysicalDevice(), &device_features);

        // This is where Acceleration Structures get generated from the secene's
        // MeshRenderers (or whatever). This is not there yet.
        AccelerationStructure structure = AccelerationStructure();
        scene = std::make_unique<TLAS>();
        (*scene).AddBLAS(&structure);
        (*scene).BuildTLAS();

        viewportWidth = width;
        viewportHeight = height;

        // Prepare Ray Tracing Pipeline
        CreateStorageImage();
        CreateViewImage();
        CreateRayTracingPipeline();
        CreateShaderBindingTables();
        CreateDescriptorSets();
        CreateCommandPool();
        BuildCommandBuffers();

        return true;
    }

    bool Backend_FullRT::Render()
    {
        VkResult err = vkGetFenceStatus(GetDevice(), drawBuffersFences[0]);
        if (err == VK_SUCCESS)
        {
            vkWaitForFences(GetDevice(), 1, &drawBuffersFences[0], true, UINT32_MAX);
            vkResetFences(GetDevice(), 1, &drawBuffersFences[0]);

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &draw_cmd_buffers[0];
            check_vk_result(vkQueueSubmit(GetRTQueue(), 1, &submit_info,  drawBuffersFences[0]));
        }
        return true;
    }

    bool Backend_FullRT::ResizeViewImage()
    {
        if (static_cast<uint32_t>(truncf(*viewportWidth)) != storage_image.width || static_cast<uint32_t>(truncf(*viewportHeight)) != storage_image.height)
        {
            // TODO: Make this use vkResetCommandBuffer or something like that (performance)
            vkWaitForFences(GetDevice(), 1, &drawBuffersFences[0], false, UINT32_MAX);
            DestroyDrawBuffers();

            // If the view port size has changed, we need to recreate the storage image
            vkDestroyImageView(GetDevice(), storage_image.view, nullptr);
            vkDestroyImage(GetDevice(), storage_image.image, nullptr);
            vkFreeMemory(GetDevice(), storage_image.memory, nullptr);
            CreateStorageImage();
            // The view image too
            vkDestroyImageView(GetDevice(), viewImage.view, nullptr);
            vkDestroyImage(GetDevice(), viewImage.image, nullptr);
            vkFreeMemory(GetDevice(), viewImage.memory, nullptr);
            vkDestroySampler(GetDevice(), viewImage.sampler, nullptr);
            CreateViewImage();
            // The descriptor also needs to be updated to reference the new image
            VkDescriptorImageInfo image_descriptor{};
            image_descriptor.imageView = storage_image.view;
            image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            VkWriteDescriptorSet result_image_write{};
            result_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            result_image_write.dstSet = descriptor_set;
            result_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            result_image_write.dstBinding = 1;
            result_image_write.pImageInfo = &image_descriptor;
            result_image_write.descriptorCount = 1;
            vkUpdateDescriptorSets(GetDevice(), 1, &result_image_write, 0, VK_NULL_HANDLE);

            BuildCommandBuffers();
            return true;
        }
        return false;
    }

    bool Backend_FullRT::CleanupBackend()
    {
        vkWaitForFences(GetDevice(), drawBuffersFences.size(), drawBuffersFences.data(), true, UINT32_MAX);

        for (size_t i = 0; i < shaderModules.size(); i++)
        {
            vkDestroyShaderModule(GetDevice(), shaderModules[i], nullptr);
        }

        std::vector<VkDescriptorSet> descSets = {descriptor_set};
        vkFreeDescriptorSets(GetDevice(), descriptor_pool, 1, descSets.data());
        vkDestroyDescriptorPool(GetDevice(), descriptor_pool, nullptr);

        DestroyDrawBuffers();
        vkDestroyCommandPool(GetDevice(), cmd_pool, nullptr);

        vkDestroyPipeline(GetDevice(), pipeline, nullptr);
        vkDestroyPipelineLayout(GetDevice(), pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(GetDevice(), descriptor_set_layout, nullptr);
        vkDestroyImageView(GetDevice(), storage_image.view, nullptr);
        vkDestroyImage(GetDevice(), storage_image.image, nullptr);
        vkFreeMemory(GetDevice(), storage_image.memory, nullptr);
        
        vkDestroyImageView(GetDevice(), viewImage.view, nullptr);
        vkDestroySampler(GetDevice(), viewImage.sampler, nullptr);
        vkDestroyImage(GetDevice(), viewImage.image, nullptr);
        vkFreeMemory(GetDevice(), viewImage.memory, nullptr);
        
        return true;
    }
#pragma endregion
}
