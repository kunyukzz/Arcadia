#include "engine/renderer/vulkan/vk_type.h"
#include "engine/core/logger.h"

void print_vk_swchain_support(void) {
	ar_INFO("Size of VkSurfaceCapabilitiesKHR: %zu bytes", sizeof(VkSurfaceCapabilitiesKHR));
	ar_INFO("Size of VkSurfaceFormatKHR: %zu bytes", sizeof(VkSurfaceFormatKHR));
	ar_INFO("Size of VkPresentModeKHR: %zu bytes", sizeof(VkPresentModeKHR));
	ar_INFO("Size of uint32_t: %zu bytes", sizeof(uint32_t));
	ar_INFO("Size of VkPhysicalDeviceProperties: %zu bytes", sizeof(VkPhysicalDeviceProperties));
	ar_INFO("Size of VkPhysicalDeviceFeatures: %zu bytes", sizeof(VkPhysicalDeviceFeatures));
	ar_INFO("Size of VkPhysicalDeviceMemoryProperties: %zu bytes", sizeof(VkPhysicalDeviceMemoryProperties));

	ar_TRACE("Size of vulkan_swapchain_support_t: %zu bytes", sizeof(vulkan_swapchain_support_t));
	
}

void print_vk_device(void) {
	ar_INFO("Size of VkPhysicalDevice: %zu bytes", sizeof(VkPhysicalDevice));
	ar_INFO("Size of VkDevice: %zu bytes", sizeof(VkDevice));
	ar_INFO("Size of VkCommandPool: %zu bytes", sizeof(VkCommandPool));

	ar_INFO("Size of VkQueue: %zu bytes", sizeof(VkQueue) * 3);
	ar_TRACE("Size of vulkan_swapchain_support_t: %zu bytes", sizeof(vulkan_swapchain_support_t));

	ar_INFO("Size of VkPhysicalDeviceProperties: %zu bytes", sizeof(VkPhysicalDeviceProperties));
	ar_INFO("Size of VkPhysicalDeviceFeatures: %zu bytes", sizeof(VkPhysicalDeviceFeatures));
	ar_INFO("Size of VkPhysicalDeviceMemoryProperties: %zu bytes", sizeof(VkPhysicalDeviceMemoryProperties));

	ar_INFO("Size of VkFormat: %zu bytes", sizeof(VkFormat));
	ar_INFO("Size of int32_t: %zu bytes", sizeof(int32_t) * 3);

	ar_TRACE("Size of vulkan_device_t: %zu bytes", sizeof(vulkan_device_t));
}

void print_vk_context(void) {
	ar_INFO("Size of VkInstance: %zu bytes", sizeof(VkInstance));
	ar_INFO("Size of VkSurfaceKHR: %zu bytes", sizeof(VkSurfaceKHR));
	ar_INFO("Size of VkAllocationCallbacks: %zu bytes", sizeof(VkAllocationCallbacks));

	ar_TRACE("Size of vulkan_context_t: %zu bytes", sizeof(vulkan_context_t));
}


void print_vulkan_structs(void) {
	print_vk_swchain_support();
	print_vk_device();
	print_vk_context();
}
