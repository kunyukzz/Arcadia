#ifndef __EVENT_H__
#define __EVENT_H__

#include "engine/define.h"

typedef struct event_context_t {
    union {
        int64_t  i64[2];
        uint64_t u64[2];
        double   f64[2];

        int32_t  i32[4];
        uint32_t u32[4];
        float    f32[4];

        int16_t  i16[8];
        uint16_t u16[8];

        int8_t   i8[16];
        uint8_t  u8[16];

        char     c[16];
    } data;
} event_context_t;

typedef b8 (*p_on_event)(uint16_t code, void *sender, void *listener,
                         event_context_t data);

typedef enum event_code_t {
    EVENT_CODE_APPLICATION_QUIT = 0x01, // shutdown application to next frame.
    EVENT_CODE_KEY_PRESSED      = 0x02, // keyboard key press
    EVENT_CODE_KEY_RELEASE      = 0x03, // keyboard key release
    EVENT_CODE_BUTTON_PRESSED   = 0x04, // mpuse button press
    EVENT_CODE_BUTTON_RELEASE   = 0x05, // mouse button release
    EVENT_CODE_MOUSE_MOVE       = 0x06, // mouse moved
    EVENT_CODE_MOUSE_WHEEL      = 0x07, // mouse wheel
    EVENT_CODE_RESIZED          = 0x08, // resize window
	EVENT_CODE_APP_SUSPEND      = 0x09, // New internal event for suspend
    EVENT_CODE_APP_RESUME       = 0x0A, // New internal event for resume
	
	EVENT_CODE_DEBUG0 			= 0x10, // Debug purpose
    MAX_EVENT_CODE              = 0xFF
} event_code_t;

void event_init(uint64_t *memory_require, void *state);
void event_shut(void *state);

b8   event_reg(uint16_t code, void *listener, p_on_event event);
b8   event_unreg(uint16_t code, void *listener, p_on_event event);
b8   event_push(uint16_t code, void *sender, event_context_t ev_context);

#endif //__EVENT_H__
