#include "engine/core/event.h"
#include "engine/memory/memory.h"
#include "engine/container/dyn_array.h"
#include "engine/core/logger.h"

typedef struct register_event_t {
	void *listener;
	p_on_event callback;
} register_event_t;

typedef struct event_entry_t {
	register_event_t *events;
} event_entry_t;

#define MAX_MSG_CODE 1024

typedef struct event_state_t {
	event_entry_t registered[MAX_MSG_CODE];
} event_state_t;

static event_state_t *p_state;

void event_init(uint64_t *memory_require, void *state) {
	*memory_require = sizeof(event_state_t);
	if (state == 0)
		return;

	memory_zero(state, sizeof(state));
	p_state = state;

	ar_INFO("Event System Initialized");
}

void event_shut(void *state) {
	(void)state;

	if (p_state) {
		for (uint16_t i = 0; i < MAX_EVENT_CODE; ++i) {
			if (p_state->registered[i].events != 0) {
				p_state->registered[i].events = 0;
			}
		}
	}

	p_state = 0;
}

b8 event_reg(uint16_t code, void *listener, p_on_event event) {
	if (!p_state)
		return false;

	if (p_state->registered[code].events == 0)
		p_state->registered[code].events = dyn_array_create(register_event_t);

	uint64_t count = dyn_array_length(p_state->registered[code].events);
	for (uint64_t i = 0; i < count; ++i) {
		if (p_state->registered[code].events[i].listener == listener)
			return false;
	}

	register_event_t reg_events;
	reg_events.listener = listener;
	reg_events.callback = event;
	dyn_array_push(p_state->registered[code].events, reg_events);

	return true;
}

b8 event_unreg(uint16_t code, void *listener, p_on_event event) {
	if (!p_state)
		return false;

	if (p_state->registered[code].events == 0)
		return false;

	uint64_t count = dyn_array_length(p_state->registered[code].events);
	for (uint64_t i = 0; i < count; ++i) {
		register_event_t e = p_state->registered[code].events[i];
		if (e.listener == listener && e.callback == event) {
			register_event_t pop;
			dyn_array_pop_at(p_state->registered[code].events, i, &pop);
			return true;
		}
	}

	return false;
}

b8 event_push(uint16_t code, void *sender, event_context_t ev_context) {
	if (!p_state)
		return false;

	if (p_state->registered[code].events == 0)
		return false;

	uint64_t count = dyn_array_length(p_state->registered[code].events);
	for (uint64_t i = 0; i < count; ++i) {
		register_event_t e = p_state->registered[code].events[i];
		if (e.callback(code, sender, e.listener, ev_context))
			return true;
	}

	return false;
}
