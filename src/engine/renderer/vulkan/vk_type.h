#ifndef __VULKAN_TYPE_H__
#define __VULKAN_TYPE_H__

#include "engine/container/free_list.h"
#include "engine/container/hashtable.h"
#include "engine/core/assertion.h"
#include "engine/renderer/renderer_type.h"


#include <vulkan/vulkan.h>

#define VK_CHECK(expr) {ar_assert(expr == VK_SUCCESS);}

struct vulkan_context_t;

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
	vec4 render_area;
	vec4 clear_color;
	float depth;
	uint32_t stencil;
	uint8_t clear_flags;
	b8 has_prev_pass;
	b8 has_next_pass;
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
	VkFramebuffer framebuffers[4];

	uint32_t image_count;
	uint8_t max_frame_in_flight;
} vulkan_swapchain_t;

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
	uint64_t freelist_mem_req;
	freelist_t buffer_freelist;
	b8 has_freelist;
	void *freelist_block;
} vulkan_buffer_t;

/* =========================== Vulkan Shaders =============================== */
typedef struct vulkan_shader_stage_t {
	VkShaderModuleCreateInfo cr_info;
	VkShaderModule handle;
	VkPipelineShaderStageCreateInfo shader_stg_cr_info;
} vulkan_shader_stage_t;

/* This for world & object shaders */
#define VULKAN_MATERIAL_MAX_COUNT 1024
#define VULKAN_UI_MAX_COUNT 1024

#define VULKAN_SHADER_MAX_STAGES 8
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31
#define VULKAN_SHADER_MAX_ATTRIBUTES 16
#define VULKAN_SHADER_MAX_UNIFORMS 128
#define VULKAN_SHADER_MAX_BINDING 32
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGE 32

typedef struct vulkan_shader_stage_config_t {
	VkShaderStageFlagBits stage;
	char filename[255];
} vulkan_shader_stage_config_t;

typedef struct vulkan_descriptor_set_config_t {
	uint8_t bind_count;
	VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDING];
} vulkan_descriptor_set_config_t;

typedef struct vulkan_shader_config_t {
	uint8_t stage_count;
	vulkan_shader_stage_config_t stages[VULKAN_SHADER_MAX_STAGES];
	VkDescriptorPoolSize pool_size[2];
	uint16_t max_desc_set_count;
	uint8_t desc_set_count;
	vulkan_descriptor_set_config_t desc_set[2];
	VkVertexInputAttributeDescription attr[VULKAN_SHADER_MAX_ATTRIBUTES];
} vulkan_shader_config_t;

typedef struct vulkan_descriptor_state_t {
	uint8_t gen[4];
	uint32_t ids[4];
} vulkan_descriptor_state_t;

typedef struct vulkan_shader_desc_set_state_t {
	VkDescriptorSet desc_sets[4];
	vulkan_descriptor_state_t desc_states[VULKAN_SHADER_MAX_BINDING];
} vulkan_shader_desc_set_state_t;

typedef struct vulkan_shader_instance_state_t {
	uint32_t id;
	uint64_t offset;
	vulkan_shader_desc_set_state_t desc_set_state;
	struct texture_t **instance_textures;
} vulkan_shader_instance_state_t;

typedef struct vulkan_shader_t {
	uint32_t id;
	vulkan_shader_config_t config;
	vulkan_renderpass_t *renderpass;
	vulkan_shader_stage_t stages[VULKAN_SHADER_MAX_STAGES];
	VkDescriptorPool desc_pool;
	VkDescriptorSetLayout desc_set_layouts[2];
	VkDescriptorSet global_desc_sets[4];
	vulkan_pipeline_t pipeline;
	vulkan_buffer_t uniform_buffer;
	uint32_t instance_count;
	vulkan_shader_instance_state_t instance_states[VULKAN_MATERIAL_MAX_COUNT];
	b8 desc_updated[4]; // IDK if this needed.
	void *map_uni_buffer_block;
} vulkan_shader_t;

/* ========================== Vulkan Geometry =============================== */
#define VULKAN_GEOMETRY_MAX_COUNT 4096

typedef struct vulkan_geo_data_t {
	uint32_t id;
	uint32_t gen;
	uint32_t vertex_count;
	uint32_t vertex_element_size;
	uint32_t vertex_buffer_offset;
	uint32_t idx_count;
	uint32_t idx_element_size;
	uint32_t idx_buffer_offset;
} vulkan_geo_data_t;

/* =========================== Vulkan Context =============================== */
typedef struct vulkan_context_t {
	VkInstance instance;
	VkSurfaceKHR surface;
	VkAllocationCallbacks *alloc;

	vulkan_device_t device;
	vulkan_swapchain_t swapchain;
	vulkan_renderpass_t main_render;
	vulkan_renderpass_t ui_render;

	vulkan_commandbuffer_t *graphic_comm_buffer;
	VkSemaphore *avail_semaphore;
	VkSemaphore *complete_semaphore;

	VkFence in_flight_fence[3];
	VkFence image_in_flight[4];
	VkFramebuffer world_framebuffer[4];

	vulkan_buffer_t obj_vert_buffer;
	vulkan_buffer_t obj_idx_buffer;

	vulkan_geo_data_t geometries[VULKAN_GEOMETRY_MAX_COUNT];

	int32_t (*find_mem_idx)(uint32_t type_filter, uint32_t prop_flag);
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
} vulkan_context_t;

typedef struct vulkan_texture_data_t {
	vulkan_image_t image;
	VkSampler sampler;
} vulkan_texture_data_t;

#endif //__VULKAN_TYPE_H__
