#ifndef __VULKAN_TYPE_H__
#define __VULKAN_TYPE_H__

#include "engine/core/assertion.h"
#include "engine/renderer/renderer_type.h"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr) {ar_assert(expr == VK_SUCCESS);}

/* ============================= Vulkan Image =============================== */
typedef struct vulkan_image_t {
	VkImage handle;
	VkDeviceMemory memory;
	VkImageView image_view;
	uint32_t width, height;
} vulkan_image_t;

/* ========================= Vulkan Renderpass ============================== */
typedef struct vulkan_renderpass_t {
	VkRenderPass handle;
	VkExtent2D extents;
} vulkan_renderpass_t;

/* ======================== Vulkan Commandbuffer ============================ */
typedef struct vulkan_framebuffer_t {
	VkFramebuffer handle;
	vulkan_renderpass_t *renderpass;
	uint32_t attach_count;
	VkImageView *attach;
} vulkan_framebuffer_t;

typedef struct vulkan_commandbuffer_t {
	VkCommandBuffer handle;
} vulkan_commandbuffer_t;

/* =========================== Vulkan Swapchain ============================= */
typedef struct vulkan_swapchain_support_t {
	VkSurfaceCapabilitiesKHR capable;
	VkSurfaceFormatKHR *formats;
	VkPresentModeKHR *present_mode;
	uint32_t format_count;
	uint32_t present_mode_count;
} vulkan_swapchain_support_t;

typedef struct vulkan_swapchain_t {
	VkSurfaceFormatKHR image_format;
	VkSwapchainKHR handle;
	VkImage *image;
	VkImageView *image_view;
	VkExtent2D extents;
	vulkan_image_t image_attach;
	vulkan_framebuffer_t *framebuffer;

	uint32_t image_count;
	uint8_t max_frame_in_flight;
} vulkan_swapchain_t;

typedef struct vulkan_fence_t {
	VkFence handle;
	b8 is_signaled;
} vulkan_fence_t;

/* ============================ Vulkan Device =============================== */
typedef struct vulkan_device_t {
	VkPhysicalDevice phys_dev;
	VkDevice logic_dev;
	VkCommandPool graphics_command_pool;

	VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

	vulkan_swapchain_support_t support;

	VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
	
	VkFormat depth_format;
	int32_t graphic_idx;
	int32_t present_idx;
	int32_t transfer_idx;
} vulkan_device_t;

/* =========================== Vulkan Context =============================== */
typedef struct vulkan_context_t {
	
	VkInstance instance;
	VkSurfaceKHR surface;
	VkAllocationCallbacks *alloc;

	vulkan_device_t device;
	vulkan_swapchain_t swapchain;
	vulkan_renderpass_t main_render;

	vulkan_commandbuffer_t *graphic_comm_buffer;
	VkSemaphore *avail_semaphore;
	VkSemaphore *complete_semaphore;

	vulkan_fence_t *in_flight_fence;
	vulkan_fence_t **image_in_flight;

	float frame_delta;

	uint64_t framebuffer_size_gen;
	uint64_t framebuffer_last_gen;

	uint32_t framebuffer_w;
	uint32_t framebuffer_h;

	uint32_t in_fligt_fence_count;
	uint32_t image_idx;
	uint32_t current_frame;

	b8 recreate_swap;

	int32_t (*find_mem_idx)(uint32_t type_filter, uint32_t prop_flag);
} vulkan_context_t;

#endif //__VULKAN_TYPE_H__
