#pragma once
#include <cstdint>
#include <VulkanHelp/vk_common.h>
#include <vector>
#include <VulkanHelp/Buffer.h>
#include <memory>
#include <glm/vec3.hpp>

namespace PBEngine
{
    struct ScratchBuffer
    {
        uint64_t       device_address;
        VkBuffer       handle;
        VkDeviceMemory memory;
    };

    class AccelerationStructure {
    public:
        AccelerationStructure(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices);
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
        void Initialize(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices);

        uint64_t get_buffer_device_address(VkBuffer buffer);
        ScratchBuffer create_scratch_buffer(VkDeviceSize size);
        void delete_scratch_buffer(ScratchBuffer& scratch_buffer);
    };
}
