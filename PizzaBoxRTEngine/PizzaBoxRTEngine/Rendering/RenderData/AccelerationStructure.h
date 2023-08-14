#pragma once
#include <cstdint>
//#include "VulkanHelp/vk_common.h"
#include "../../External/volk/volk.h"
//#include <vulkan/vulkan.h>
//#include <vulkan/vulkan.hpp>
#include <vector>

namespace PBEngine
{
    struct Vertex {
        float x, y, z;
    };

    struct Index {
        uint32_t index;
    };

    // Define the vertex data for the triangle
    const std::vector<float> triangleVertices = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    class AccelerationStructure {
    public:
        AccelerationStructure();
        AccelerationStructure(AccelerationStructure&&) = delete;
        AccelerationStructure(const AccelerationStructure&) = delete;
        ~AccelerationStructure();

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        VkAccelerationStructureKHR accelStruct;
        VkAccelerationStructureCreateInfoKHR accelStructureInfo;
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
        VkAccelerationStructureGeometryTrianglesDataKHR geometryData = {};
    };
}
