#ifndef __RENDERER_BACKEND_H__
#define __RENDERER_BACKEND_H__

#include "engine/renderer/renderer_type.h"

struct platform_state_t;

b8 renderer_be_init(render_backend_type_t type, render_backend_t *backend);
void renderer_be_shut(render_backend_t *backend);

#endif //__RENDERER_BACKEND_H__
