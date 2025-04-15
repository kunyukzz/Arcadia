#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "engine/define.h"

struct game_entry;

typedef struct application_config_t {
	int32_t pos_x, pos_y;
	uint32_t width, height;
	char *name;
} application_config_t;

b8 application_init(struct game_entry *game_inst);
b8 application_run(void);
void application_get_framebuffer_size(uint32_t *width, uint32_t *height);

#endif //__APPLICATION_H__
