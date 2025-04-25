#ifndef __VK_PLATFORM_H__
#define __VK_PLATFORM_H__

#include "engine/define.h"

struct platform_state_t;
struct vulkan_context_t;

b8 platform_create_vulkan_surface(struct vulkan_context_t* context);

#endif //__VK_PLATFORM_H__
