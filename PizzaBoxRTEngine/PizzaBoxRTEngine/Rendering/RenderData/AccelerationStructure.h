#pragma once
#include <cstdint>
#include <VulkanHelp/vk_common.h>
#include <vector>
#include <VulkanHelp/Buffer.h>
#include <memory>

namespace PBEngine
{
    struct ScratchBuffer
    {
        uint64_t       device_address;
        VkBuffer       handle;
        VkDeviceMemory memory;
    };

    struct Vertex
    {
        float pos[3];
    };

    // Define the vertex data for the triangle
    const std::vector<Vertex> triangleVertices = {
        {{-0.5f, -0.5f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}},
        {{0.0f,  0.5f, 0.0f}}};
    const std::vector<uint32_t> indices = { 0, 1, 2 };

    class AccelerationStructure {
    public:
        AccelerationStructure();
        AccelerationStructure(AccelerationStructure&&);
        AccelerationStructure(const AccelerationStructure&) = delete;
        ~AccelerationStructure();

        VkAccelerationStructureKHR handle;
        uint64_t deviceAddress;
        std::unique_ptr<Buffer> buffer;

        std::unique_ptr<Buffer> vertexBuffer;
        std::unique_ptr<Buffer> indexBuffer;
        uint32_t indexCount;

    private:
        uint64_t get_buffer_device_address(VkBuffer buffer);
        ScratchBuffer create_scratch_buffer(VkDeviceSize size);
        void delete_scratch_buffer(ScratchBuffer& scratch_buffer);
    };
}
