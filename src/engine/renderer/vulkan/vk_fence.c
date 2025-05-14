#include "engine/renderer/vulkan/vk_fence.h"

#include "engine/core/logger.h"
#include <vulkan/vulkan_core.h>

void vk_fence_init(vulkan_context_t *ctx, b8 signaling, vulkan_fence_t *fence) {
	fence->is_signaled = signaling;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (fence->is_signaled)
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(ctx->device.logic_dev, &fence_info, ctx->alloc,
                           &fence->handle));
}

void vk_fence_shut(vulkan_context_t *ctx, vulkan_fence_t *fence) {
	if (fence->handle) {
		vkDestroyFence(ctx->device.logic_dev, fence->handle, ctx->alloc);
		fence->handle = 0;
	}

	fence->is_signaled = false;
}

b8 vk_fence_wait(vulkan_context_t *ctx, vulkan_fence_t *fence,
                 uint64_t timeout) {
    if (!fence->is_signaled) {
        // BUG: missleading validation message
        // TODO: GitHub Issue #5589
        // ADDED QUEUE WAIT IDLE TO MISLEADING MESSAGE FROM VULKAN VALIDATION BUG!!!
        vkQueueWaitIdle(ctx->device.graphics_queue);

        VkResult result = vkWaitForFences(ctx->device.logic_dev, 1,
                                          &fence->handle, true, timeout);
        switch (result) {
        case VK_SUCCESS:
            fence->is_signaled = true;
            return true;
        case VK_TIMEOUT:
            ar_WARNING("vk_fence_wait - Time out");
            break;
        case VK_ERROR_DEVICE_LOST:
            ar_ERROR("vk_fence_wait - VK_ERROR_DEVICE_LOST");
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ar_ERROR("vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            ar_ERROR("vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY");
            break;
        default:
            ar_ERROR("vk_fence_wait - An unknown error has occurred") break;
        }
    } else {
        return true;
    }
    return false;
}

void vk_fence_reset(vulkan_context_t *ctx, vulkan_fence_t *fence) {
	if (fence->is_signaled) {
		VK_CHECK(vkResetFences(ctx->device.logic_dev, 1, &fence->handle));
		fence->is_signaled = false;
	}
}
