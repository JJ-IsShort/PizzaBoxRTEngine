#pragma once
#include "vk_common.h"
#include <vector>

namespace PBEngine
{
	class Buffer
	{
	public:
		/**
		 * @brief Creates a buffer
		 * @param g_Device A valid Vulkan device
		 * @param g_PhysicalDevice A valid Vulkan physical device
		 * @param size The size in bytes of the buffer
		 * @param usage The usage flags for the VkBuffer
		 * @param properties The memory usage of the buffer
		 */
		Buffer(VkDevice g_Device, VkPhysicalDevice g_PhysicalDevice, VkDeviceSize size,
			VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		Buffer(VkDevice g_Device, VkPhysicalDevice g_PhysicalDevice, VkDeviceSize size,
			VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBufferCreateFlags& bufferCreateFlags);
		Buffer() {};
		Buffer(const Buffer&) = delete;
		Buffer(Buffer&& other);
		~Buffer();

		Buffer& operator=(const Buffer&) = delete;
		Buffer& operator=(Buffer&&) = delete;

		static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

		const VkBuffer* get() const;
		VkBuffer get_handle();
		VkDeviceMemory get_memory() const;
		VkDeviceSize get_size();

		/**
		* @brief Maps buffer if it isn't already mapped to a host visible address.
		* @return Pointer to host visible memory
		*/
		void* map();

		/**
		* @brief Unmaps vulkan memory from the host visible address
		*/
		void unmap();

		const void* get_data() const;

		/**
		 * @brief Copies byte data into the buffer
		 * @param data The data to copy from
		 * @param size The amount of bytes to copy
		 * @param offset The offset to start the copying into the mapped data
		 */
		void update(const uint8_t* data, size_t size, size_t offset = 0);

		/**
		 * @brief Converts any non byte data into bytes and then updates the buffer
		 * @param data The data to copy from
		 * @param size The amount of bytes to copy
		 * @param offset The offset to start the copying into the mapped data
		 */
		void update(void* data, size_t size, size_t offset = 0);

		/**
		 * @brief Copies a vector of bytes into the buffer
		 * @param data The data vector to upload
		 * @param offset The offset to start the copying into the mapped data
		 */
		void update(const std::vector<uint8_t>& data, size_t offset = 0);

		/**
		 * @brief Copies an object as byte data into the buffer
		 * @param object The object to convert into byte data
		 * @param offset The offset to start the copying into the mapped data
		 */
		template <class T>
		void convert_and_update(const T& object, size_t offset = 0);

		/**
		 * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
		 */
		uint64_t get_device_address();

	private:
		VkBuffer buffer;
		VkDeviceMemory bufferMemory;

		VkDevice device;
		VkPhysicalDevice physicalDevice;

		VkDeviceSize bufferSize;

		bool mapped{ false };
		void* mapped_data{ nullptr };
	};
}