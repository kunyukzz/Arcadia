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
typedef enum vulkan_combuff_state_t {
    _STATE_READY,
    _STATE_RECORDING,
    _STATE_IN_RENDERPASS,
    _STATE_RECORD_END,
    _STATE_SUBMITTED,
    _STATE_NOT_ALLOCATED
} vulkan_combuff_state_t;

typedef struct vulkan_framebuffer_t {
	VkFramebuffer handle;
	vulkan_renderpass_t *renderpass;
	uint32_t attach_count;
	VkImageView *attach;
} vulkan_framebuffer_t;

typedef struct vulkan_commandbuffer_t {
	VkCommandBuffer handle;
	vulkan_combuff_state_t state;
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

	b8 support_dev_local_host_vsb;
} vulkan_device_t;

/* ========================== Vulkan Pipeline =============================== */
typedef struct vulkan_pipeline_t {
	VkPipeline handle;
	VkPipelineLayout pipe_layout;
} vulkan_pipeline_t;

/* =========================== Vulkan Buffers =============================== */
typedef struct vulkan_buffer_t {
	uint64_t total_size;
	VkBuffer handle;
	VkBufferUsageFlagBits usage;
	b8 is_locked;
	VkDeviceMemory memory;
	int32_t mem_idx;
	uint32_t mem_prop_flag;
} vulkan_buffer_t;

/* =========================== Vulkan Shaders =============================== */
typedef struct vulkan_shader_stage_t {
	VkShaderModuleCreateInfo cr_info;
	VkShaderModule handle;
	VkPipelineShaderStageCreateInfo shader_stg_cr_info;
} vulkan_shader_stage_t;

#define OBJ_SHADER_STAGE_COUNT 2
#define VULKAN_SHADER_DESC_COUNT 2
#define VULKAN_MAX_OBJ_COUNT 1024

typedef struct vulkan_desc_state_t {
	uint32_t gen[4];
	uint32_t id[4];
} vulkan_desc_state_t;

typedef struct vulkan_shader_obj_state_t {
	VkDescriptorSet desc_sets[4];
	vulkan_desc_state_t desc_states[VULKAN_SHADER_DESC_COUNT];
} vulkan_shader_obj_state_t;

typedef struct vulkan_material_shader_t {
	vulkan_shader_stage_t stages[OBJ_SHADER_STAGE_COUNT];
	vulkan_pipeline_t pipeline;
	vulkan_buffer_t global_uni_buffer;
	vulkan_buffer_t obj_uni_buffer;
	vulkan_shader_obj_state_t obj_states[VULKAN_MAX_OBJ_COUNT];

	VkDescriptorSetLayout global_desc_set_layout;
	VkDescriptorPool global_desc_pool;
	VkDescriptorSet global_desc_sets[4]; // TODO: configure this using dynamic
										 // array

	VkDescriptorSetLayout obj_desc_set_layout;
	VkDescriptorPool obj_desc_pool;

	global_uni_obj_t global_ubo;

	uint32_t obj_uniform_buffer_idx;
	b8 desc_updated[4]; // TODO: configure this using dynamic array
} vulkan_material_shader_t;

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

	vulkan_buffer_t obj_vert_buffer;
	vulkan_buffer_t obj_idx_buffer;

	float frame_delta;

	uint64_t geo_vert_offset;
	uint64_t geo_idx_offset;

	uint64_t framebuffer_size_gen;
	uint64_t framebuffer_last_gen;

	uint32_t framebuffer_w;
	uint32_t framebuffer_h;

	uint32_t in_fligt_fence_count;
	uint32_t image_idx;
	uint32_t current_frame;

	b8 recreate_swap;
	vulkan_material_shader_t obj_shader;

	int32_t (*find_mem_idx)(uint32_t type_filter, uint32_t prop_flag);
} vulkan_context_t;

typedef struct vulkan_texture_data_t {
	vulkan_image_t image;
	VkSampler sampler;
} vulkan_texture_data_t;

#endif //__VULKAN_TYPE_H__
