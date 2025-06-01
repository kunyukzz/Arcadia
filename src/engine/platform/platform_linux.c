#include "engine/platform/platform.h"

#if OS_LINUX

#include "engine/core/logger.h"
#include "engine/core/keycode.h"
#include "engine/core/event.h"
#include "engine/core/input.h"
#include "engine/core/ar_strings.h"

#include "engine/resources/icon_data.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#define VK_USE_PLATFORM_XCB_KHR
#include "engine/renderer/vulkan/vk_type.h"
#include <vulkan/vulkan.h>
#include <xcb/randr.h>

#include <stdlib.h>
#include <unistd.h>

typedef struct platform_state_t {
	Display *display;
	xcb_connection_t *conn;
	xcb_screen_t *screen;
	xcb_window_t window;

	xcb_atom_t wm_protocol;
	xcb_atom_t wm_delete_win;
	xcb_atom_t wm_state;

	VkSurfaceKHR surface;
} platform_state_t;

static platform_state_t *p_state;

keys translate_keycode(uint32_t x_keycode);

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
void set_window_icon(xcb_connection_t *conn, uint32_t *data, uint16_t icon_len) {
    xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(conn, 0, string_length("_NET_WM_ICON"), "_NET_WM_ICON");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
    if (!reply)
        return;

    xcb_atom_t net_wm_icon = reply->atom;
    free(reply);

    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, p_state->window,
                        net_wm_icon, XCB_ATOM_CARDINAL, 32, icon_len,
                        data);

    // Some WMs expect _NET_WM_PID too
    xcb_intern_atom_cookie_t pid_cookie =
        xcb_intern_atom(conn, 0, strlen("_NET_WM_PID"), "_NET_WM_PID");
    xcb_intern_atom_reply_t *pid_reply =
        xcb_intern_atom_reply(conn, pid_cookie, NULL);
    if (pid_reply) {
        xcb_atom_t net_wm_pid = pid_reply->atom;
        free(pid_reply);
        uint32_t pid = (uint32_t)getpid();
        xcb_change_property(conn, XCB_PROP_MODE_REPLACE, p_state->window,
                            net_wm_pid, XCB_ATOM_CARDINAL, 32, 1, &pid);
    }
}

/* Add implementation for considering multi monitor */
b8 get_monitor_under_cursor(xcb_connection_t *conn, xcb_screen_t *screen,
    int *x_out, int *y_out, uint32_t *width_out, uint32_t *height_out) {
    if (!conn || !screen || !x_out || !y_out || !width_out || !height_out) {
        return false;
    }
    
    xcb_query_pointer_reply_t *pointer = xcb_query_pointer_reply(conn,
        xcb_query_pointer(conn, screen->root), NULL);
    if (!pointer) { return false;}
    
    int mouse_x = pointer->root_x;
    int mouse_y = pointer->root_y;
    free(pointer);
    xcb_randr_get_screen_resources_reply_t *res =
    xcb_randr_get_screen_resources_reply(conn,
        xcb_randr_get_screen_resources(conn, screen->root), NULL);
    if (!res) {
        return false;
    }

    xcb_randr_crtc_t *crtcs = xcb_randr_get_screen_resources_crtcs(res);
    for (int i = 0; i < res->num_crtcs; ++i) {
        xcb_randr_get_crtc_info_reply_t *crtc_info =
            xcb_randr_get_crtc_info_reply(conn,
                xcb_randr_get_crtc_info(conn, crtcs[i], XCB_CURRENT_TIME), NULL);

        if (crtc_info &&
            mouse_x >= crtc_info->x &&
            mouse_x < crtc_info->x + crtc_info->width &&
            mouse_y >= crtc_info->y &&
            mouse_y < crtc_info->y + crtc_info->height) {

            *x_out = crtc_info->x;
            *y_out = crtc_info->y;
            *width_out = crtc_info->width;
            *height_out = crtc_info->height;

            free(crtc_info);
            free(res);
            return true;
        }

        free(crtc_info);
    }

    free(res);
    return false;
}
/* ========================================================================== */
/* ========================================================================== */

b8 platform_init(uint64_t *memory_require, void *state, const char *name,
                 int32_t x, int32_t y, uint32_t w, uint32_t h) {
    *memory_require = sizeof(platform_state_t);
	if (state == 0) return true;

	p_state = state;

	p_state->display = XOpenDisplay(NULL);
	p_state->conn = XGetXCBConnection(p_state->display);
	const struct xcb_setup_t *setup = xcb_get_setup(p_state->conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	p_state->screen = iter.data;
	p_state->window = xcb_generate_id(p_state->conn);

	uint32_t event_mask =
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
	uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t value_list[] = {p_state->screen->black_pixel, event_mask};

    int mon_x, mon_y;
    uint32_t mon_w, mon_h;
    if (get_monitor_under_cursor(p_state->conn, p_state->screen, &mon_x, &mon_y,
                                 &mon_w, &mon_h)) {
        x = mon_x + (int32_t)((mon_w - w) / 2);
        y = mon_y + (int32_t)((mon_h - h) / 2);
    } else {
        // fallback to default screen centering
        x = (p_state->screen->width_in_pixels - w) / 2;
        y = (p_state->screen->height_in_pixels - h) / 2;
    }

    xcb_create_window(p_state->conn, XCB_COPY_FROM_PARENT, p_state->window,
                    p_state->screen->root, x, y, w, h, 0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, p_state->screen->root_visual,
                    value_mask, value_list);

    xcb_change_property(p_state->conn, XCB_PROP_MODE_REPLACE, p_state->window,
                        XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                        string_length(name), name);

    xcb_intern_atom_cookie_t del_cookie = xcb_intern_atom(p_state->conn, 0,
			string_length("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_cookie =
        xcb_intern_atom(p_state->conn, 0, string_length("WM_STATE"), "WM_STATE");
    xcb_intern_atom_cookie_t proto_cookie =
        xcb_intern_atom(p_state->conn, 0, string_length("WM_PROTOCOLS"),
                        "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *del_reply =
		xcb_intern_atom_reply(p_state->conn, del_cookie, NULL);
	xcb_intern_atom_reply_t *proto_reply =
		xcb_intern_atom_reply(p_state->conn, proto_cookie, NULL);
    xcb_intern_atom_reply_t *wm_reply =
        xcb_intern_atom_reply(p_state->conn, wm_cookie, NULL);

    p_state->wm_delete_win = del_reply->atom;
	p_state->wm_protocol = proto_reply->atom;
	p_state->wm_state = wm_reply->atom;

	xcb_change_property(p_state->conn, XCB_PROP_MODE_REPLACE, p_state->window,
			proto_reply->atom, 4, 32, 1, &del_reply->atom);

    set_window_icon(p_state->conn, (uint32_t *)icon_data,
                    (uint16_t)icon_data_len);

    free(del_reply);
	free(proto_reply);
	free(wm_reply);

	xcb_map_window(p_state->conn, p_state->window);

	uint32_t values[] = {(uint32_t)x, (uint32_t)y};
	xcb_configure_window(p_state->conn, p_state->window,
			XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);

	xcb_flush(p_state->conn);

	ar_INFO("Platform System Initialized");
	return true;
}

b8 is_window_minimized(xcb_connection_t *conn, xcb_window_t window) {
    xcb_get_property_cookie_t cookie =
        xcb_get_property(conn, 0, window, p_state->wm_state, p_state->wm_state,
                         0, 2);
    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(conn, cookie, NULL);
    if (reply) {
        if (reply->type == p_state->wm_state && reply->format == 32 &&
            reply->value_len >= 2) {
            uint32_t *state  = (uint32_t *)xcb_get_property_value(reply);
            b8        result = (state[0] == XCB_ICCCM_WM_STATE_ICONIC);
            free(reply);
            return result;
        }
        free(reply);
    }
    return false;
}

void platform_shut(void *state) {
	(void)state;
	if (p_state) {
		xcb_destroy_window(p_state->conn, p_state->window);
		xcb_flush(p_state->conn);
		XCloseDisplay(p_state->display);
	}

	p_state = 0;
}

b8 platform_push(void) {
	if (p_state) {
		xcb_generic_event_t *ev;
		xcb_client_message_event_t *cm;
		b8 quit_flag = false;

		while ((ev = xcb_poll_for_event(p_state->conn))) {
			if (ev == 0) break;

			switch (ev->response_type & ~0x80) {
				case XCB_KEY_PRESS:
				case XCB_KEY_RELEASE: {
					xcb_key_press_event_t *kb_ev = (xcb_key_press_event_t *)ev;
					b8 pressed = ev->response_type == XCB_KEY_PRESS;
					xcb_keycode_t code = kb_ev->detail;
					KeySym key_sym = XkbKeycodeToKeysym(
						p_state->display, (KeyCode)code, 0,
						code & ShiftMask ? 1 : 0);
					keys key = translate_keycode(key_sym);
					input_process_key(key, pressed);
				} break;

				case XCB_BUTTON_PRESS:
				case XCB_BUTTON_RELEASE: {
					xcb_button_press_event_t *mouse_ev =
						(xcb_button_press_event_t *)ev;
					b8 pressed = ev->response_type == XCB_BUTTON_PRESS;
					buttons mouse_button = BUTTON_MAX;
					switch (mouse_ev->detail) {
						case XCB_BUTTON_INDEX_1:
							mouse_button = BUTTON_LEFT; break;
						case XCB_BUTTON_INDEX_2:
							mouse_button = BUTTON_MIDDLE; break;
						case XCB_BUTTON_INDEX_3:
							mouse_button = BUTTON_RIGHT; break;
					}

					if (mouse_button != BUTTON_MAX)
						input_process_button(mouse_button, pressed);
				} break;

				case XCB_MOTION_NOTIFY: {
					xcb_motion_notify_event_t *move_ev =
						(xcb_motion_notify_event_t *)ev;
					input_process_mouse_move(move_ev->event_x, move_ev->event_y);
				} break;

				case XCB_CONFIGURE_NOTIFY: {
					xcb_configure_notify_event_t *cfg_ev =
						(xcb_configure_notify_event_t *)ev;

					event_context_t c;
					c.data.u16[0] = cfg_ev->width;
					c.data.u16[1] = cfg_ev->height;

					event_push(EVENT_CODE_RESIZED, 0, c);
				} break;

				case XCB_UNMAP_NOTIFY:
				case XCB_MAP_NOTIFY: {

                    event_context_t c = {0};
                    if (is_window_minimized(p_state->conn, p_state->window)) {
						event_push(EVENT_CODE_APP_SUSPEND, 0, c);
					} else {
						event_push(EVENT_CODE_APP_RESUME, 0, c);
					}
				} break;

                case XCB_CLIENT_MESSAGE: {
					cm = (xcb_client_message_event_t *)ev;

					if (cm->data.data32[0] == p_state->wm_delete_win) {
						quit_flag = true;

						event_context_t c;
						c.data.u8[0] = cm->window;
						event_push(EVENT_CODE_APPLICATION_QUIT, 0, c);
					}
				} break;
				default: break;
			}
			free(ev);
		}
		return !quit_flag;
	}

	return true;
}

void *platform_allocate(uint64_t size, b8 aligned) {
	(void)aligned;
	return malloc(size);
}

void platform_free(void *block, b8 aligned) {
	(void)aligned;
	free(block);
}

void* platform_zero_mem(void* block, uint64_t size) {
	return memset(block, 0, size);
}

void* platform_copy_mem(void* dest, const void* source, uint64_t size) {
	return memcpy(dest, source, size);
}

void* platform_set_mem(void* dest, int32_t value, uint64_t size) {
	return memset(dest, value, size);
}

b8 platform_create_vulkan_surface(struct vulkan_context_t *context) {
	if (!p_state)
		return false;

	VkXcbSurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	create_info.connection = p_state->conn;
	create_info.window = p_state->window;

	VkResult result = vkCreateXcbSurfaceKHR(context->instance, &create_info,
											context->alloc, &context->surface);
	if (result != VK_SUCCESS) {
		ar_FATAL("Vulkan Surface failed to create");
		return false;
	}

	/* this it to hold surface inside context to pass into p_state. not
	 * neccessarily needed this unless want it. */
	//p_state->surface = context->surface;
	//context->surface = p_state->surface;
	return true;
}

keys translate_keycode(uint32_t x_keycode)
{
    switch (x_keycode)
    {
    case XK_BackSpace:
        return KEY_BACKSPACE;
    case XK_Return:
        return KEY_ENTER;
    case XK_Tab:
        return KEY_TAB;
        // case XK_Shift: return KEY_SHIFT;
        // case XK_Control: return KEY_CONTROL;
    case XK_Pause:
        return KEY_PAUSE;
    case XK_Caps_Lock:
        return KEY_CAPITAL;
    case XK_Escape:
        return KEY_ESCAPE;
        // Not supported
        // case : return KEY_CONVERT;
        // case : return KEY_NONCONVERT;
        // case : return KEY_ACCEPT;
    case XK_Mode_switch:
        return KEY_MODECHANGE;
    case XK_space:
        return KEY_SPACE;
    case XK_Prior:
        return KEY_PRIOR;
    case XK_Next:
        return KEY_NEXT;
    case XK_End:
        return KEY_END;
    case XK_Home:
        return KEY_HOME;
    case XK_Left:
        return KEY_LEFT;
    case XK_Up:
        return KEY_UP;
    case XK_Right:
        return KEY_RIGHT;
    case XK_Down:
        return KEY_DOWN;
    case XK_Select:
        return KEY_SELECT;
    case XK_Print:
        return KEY_PRINT;
    case XK_Execute:
        return KEY_EXECUTE;
    // case XK_snapshot: return KEY_SNAPSHOT; // not supported
    case XK_Insert:
        return KEY_INSERT;
    case XK_Delete:
        return KEY_DELETE;
    case XK_Help:
        return KEY_HELP;
    case XK_Meta_L:
        return KEY_LWIN; // TODO: not sure this is right
    case XK_Meta_R:
        return KEY_RWIN;
        // case XK_apps: return KEY_APPS; // not supported
        // case XK_sleep: return KEY_SLEEP; //not supported
    case XK_KP_0:
        return KEY_NUMPAD0;
    case XK_KP_1:
        return KEY_NUMPAD1;
    case XK_KP_2:
        return KEY_NUMPAD2;
    case XK_KP_3:
        return KEY_NUMPAD3;
    case XK_KP_4:
        return KEY_NUMPAD4;
    case XK_KP_5:
        return KEY_NUMPAD5;
    case XK_KP_6:
        return KEY_NUMPAD6;
    case XK_KP_7:
        return KEY_NUMPAD7;
    case XK_KP_8:
        return KEY_NUMPAD8;
    case XK_KP_9:
        return KEY_NUMPAD9;
    case XK_multiply:
        return KEY_MULTIPLY;
    case XK_KP_Add:
        return KEY_ADD;
    case XK_KP_Separator:
        return KEY_SEPARATOR;
    case XK_KP_Subtract:
        return KEY_SUBTRACT;
    case XK_KP_Decimal:
        return KEY_DECIMAL;
    case XK_KP_Divide:
        return KEY_DIVIDE;
    case XK_F1:
        return KEY_F1;
    case XK_F2:
        return KEY_F2;
    case XK_F3:
        return KEY_F3;
    case XK_F4:
        return KEY_F4;
    case XK_F5:
        return KEY_F5;
    case XK_F6:
        return KEY_F6;
    case XK_F7:
        return KEY_F7;
    case XK_F8:
        return KEY_F8;
    case XK_F9:
        return KEY_F9;
    case XK_F10:
        return KEY_F10;
    case XK_F11:
        return KEY_F11;
    case XK_F12:
        return KEY_F12;
    case XK_F13:
        return KEY_F13;
    case XK_F14:
        return KEY_F14;
    case XK_F15:
        return KEY_F15;
    case XK_F16:
        return KEY_F16;
    case XK_F17:
        return KEY_F17;
    case XK_F18:
        return KEY_F18;
    case XK_F19:
        return KEY_F19;
    case XK_F20:
        return KEY_F20;
    case XK_F21:
        return KEY_F21;
    case XK_F22:
        return KEY_F22;
    case XK_F23:
        return KEY_F23;
    case XK_F24:
        return KEY_F24;
    case XK_Num_Lock:
        return KEY_NUMLOCK;
    case XK_Scroll_Lock:
        return KEY_SCROLL;
    case XK_KP_Equal:
        return KEY_NUMPAD_EQUAL;
    case XK_Shift_L:
        return KEY_LSHIFT;
    case XK_Shift_R:
        return KEY_RSHIFT;
    case XK_Control_L:
        return KEY_LCONTROL;
    case XK_Control_R:
        return KEY_RCONTROL;
    // case XK_Menu: return KEY_LMENU;
    case XK_Alt_L:
        return KEY_LALT;
    case XK_Alt_R:
        return KEY_RALT;

    case XK_semicolon:
        return KEY_SEMICOLON;
    case XK_plus:
        return KEY_PLUS;
    case XK_comma:
        return KEY_COMMA;
    case XK_minus:
        return KEY_MINUS;
    case XK_period:
        return KEY_PERIOD;
    case XK_slash:
        return KEY_SLASH;
    case XK_grave:
        return KEY_GRAVE;
    case XK_a:
    case XK_A:
        return KEY_A;
    case XK_b:
    case XK_B:
        return KEY_B;
    case XK_c:
    case XK_C:
        return KEY_C;
    case XK_d:
    case XK_D:
        return KEY_D;
    case XK_e:
    case XK_E:
        return KEY_E;
    case XK_f:
    case XK_F:
        return KEY_F;
    case XK_g:
    case XK_G:
        return KEY_G;
    case XK_h:
    case XK_H:
        return KEY_H;
    case XK_i:
    case XK_I:
        return KEY_I;
    case XK_j:
    case XK_J:
        return KEY_J;
    case XK_k:
    case XK_K:
        return KEY_K;
    case XK_l:
    case XK_L:
        return KEY_L;
    case XK_m:
    case XK_M:
        return KEY_M;
    case XK_n:
    case XK_N:
        return KEY_N;
    case XK_o:
    case XK_O:
        return KEY_O;
    case XK_p:
    case XK_P:
        return KEY_P;
    case XK_q:
    case XK_Q:
        return KEY_Q;
    case XK_r:
    case XK_R:
        return KEY_R;
    case XK_s:
    case XK_S:
        return KEY_S;
    case XK_t:
    case XK_T:
        return KEY_T;
    case XK_u:
    case XK_U:
        return KEY_U;
    case XK_v:
    case XK_V:
        return KEY_V;
    case XK_w:
    case XK_W:
        return KEY_W;
    case XK_x:
    case XK_X:
        return KEY_X;
    case XK_y:
    case XK_Y:
        return KEY_Y;
    case XK_z:
    case XK_Z:
        return KEY_Z;
    default:
        return 0;
    }
}

#endif

