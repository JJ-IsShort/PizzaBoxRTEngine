#import "stdint.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;

namespace VkObjects {
// Code taken from
// https://github.com/yknishidate/single-file-vulkan-pathtracing/blob/master/main.cpp#L28
// Thanks so so much
// Might rewrite later, because it really doesn't work
struct Context {
  Context() {
    // Create window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window =
        glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Pathtracing", nullptr, nullptr);

    // Create instance
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<std::string> extensions(glfwExtensions,
                                        glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::vector<std::string> layers{"VK_LAYER_KHRONOS_validation",
                                    "VK_LAYER_LUNARG_monitor"};

    auto vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    auto appInfo = vk::ApplicationInfo().setApiVersion(VK_API_VERSION_1_3);

    instance = vk::createInstanceUnique({{}, &appInfo, layers, extensions});
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    physicalDevice = instance->enumeratePhysicalDevices().front();

    // Create debug messenger
    messenger = instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
            .setPfnUserCallback(&debugUtilsMessengerCallback));

    // Create surface
    VkSurfaceKHR _surface;
    VkResult res = glfwCreateWindowSurface(VkInstance(*instance), window,
                                           nullptr, &_surface);
    if (res != VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(_surface), {*instance});

    // Find queue family
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    for (int i = 0; i < queueFamilies.size(); i++) {
      auto supportCompute =
          queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute;
      auto supportPresent = physicalDevice.getSurfaceSupportKHR(i, *surface);
      if (supportCompute && supportPresent) {
        queueFamily = i;
      }
    }

    // Create device
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        {}, queueFamily, 1, &queuePriority};

    const std::vector deviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    };

    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::DeviceCreateInfo createInfo{
        {}, queueCreateInfo, {}, deviceExtensions, &deviceFeatures};
    vk::StructureChain<vk::DeviceCreateInfo,
                       vk::PhysicalDeviceBufferDeviceAddressFeatures,
                       vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                       vk::PhysicalDeviceAccelerationStructureFeaturesKHR>
        createInfoChain{createInfo, {true}, {true}, {true}};

    device = physicalDevice.createDeviceUnique(
        createInfoChain.get<vk::DeviceCreateInfo>());
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

    queue = device->getQueue(queueFamily, 0);

    // Create command pool
    commandPool = device->createCommandPoolUnique(
        vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(queueFamily));

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        {vk::DescriptorType::eAccelerationStructureKHR, 1},
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eStorageBuffer, 3}};

    descPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo()
            .setPoolSizes(poolSizes)
            .setMaxSets(1)
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet));
  }

  uint32_t findMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memProperties =
        physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i != memProperties.memoryTypeCount; ++i) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) ==
              properties) {
        return i;
      }
    }
    throw std::runtime_error("failed to find suitable memory type");
  }

  void oneTimeSubmit(const std::function<void(vk::CommandBuffer)> &func) {
    vk::UniqueCommandBuffer commandBuffer = std::move(
        device
            ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo()
                                               .setCommandPool(*commandPool)
                                               .setCommandBufferCount(1))
            .front());
    commandBuffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    func(*commandBuffer);
    commandBuffer->end();

    queue.submit(vk::SubmitInfo().setCommandBuffers(*commandBuffer));
    queue.waitIdle();
  }

  vk::UniqueDescriptorSet
  allocateDescSet(vk::DescriptorSetLayout descSetLayout) {
    return std::move(
        device->allocateDescriptorSetsUnique({*descPool, descSetLayout})
            .front());
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageTypes,
      VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
      void *pUserData) {
    std::cerr << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  GLFWwindow *window;
  vk::DynamicLoader dl;
  vk::UniqueInstance instance;
  vk::UniqueDebugUtilsMessengerEXT messenger;
  vk::UniqueSurfaceKHR surface;
  vk::UniqueDevice device;
  vk::PhysicalDevice physicalDevice;
  uint32_t queueFamily;
  vk::Queue queue;
  vk::UniqueCommandPool commandPool;
  vk::UniqueDescriptorPool descPool;
};
} // namespace VkObjects
