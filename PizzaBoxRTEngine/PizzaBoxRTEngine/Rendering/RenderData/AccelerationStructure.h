#pragma once
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>

class AccelerationStructure {
public:
  AccelerationStructure();
  AccelerationStructure(AccelerationStructure &&) = delete;
  AccelerationStructure(const AccelerationStructure &) = delete;
  ~AccelerationStructure();

  AccelStructureData data;

  struct AccelStructureData {
    VkAccelerationStructureKHR handle;
    uint64_t device_address;
    std::unique_ptr<vk::Buffer> buffer;
  };
};
