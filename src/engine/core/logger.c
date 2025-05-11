#include "logger.h"
#include "engine/core/assertion.h"
#include "engine/core/ar_strings.h"

#include "engine/platform/filesystem.h"
#include "engine/memory/memory.h"

#define LENGTH 32000

#if OS_LINUX
	#define WHITE 		"38;5;15"
	#define RED 		"38;5;196"
	#define ORANGE 		"38;5;208"
	#define YELLOW 		"38;5;220"
	#define GREEN 		"38;5;040"
	#define CYAN 		"38;5;050"
	#define MAGENTA 	"38;5;219"
#elif OS_WINDOWS
	#define WHITE 		0x0F
	#define RED 		0x0C
	#define ORANGE 		0x06
	#define YELLOW 		0x0E
	#define GREE 		0x0A
	#define CYAN    	0x0B
	#define MAGENTA 	0x0D
#endif

typedef struct log_state_t {
	file_handle_t log_handle;
} log_state_t;

static log_state_t *p_state;


void console_write(const char *msg, uint8_t color) {
#if OS_LINUX
    const char *color_string[] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN, 		// DEBUG
		MAGENTA 	// TRACE
	};

    printf("\033[%sm%s\033[0m", color_string[color], msg);

#elif OS_WINDOWS
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	static uint8_t types[6] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN, 		// DEBUG
		MAGENTA 	// TRACE
	};

	SetConsoleTextAttribute(console_handle, types[color]);

	OutputDebugStringA(msg);
	uint64_t length = strlen(msg);
	LPDWORD num_written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), msg, (DWORD)length,
				  num_written, 0);
#endif
}

void console_write_error(const char *msg, uint8_t color) {
#if OS_LINUX
    const char *color_string[] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN, 		// DEBUG
		MAGENTA 	// TRACE
	};

    printf("\033[%sm%s\033[0m", color_string[color], msg);

#elif OS_WINDOWS
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	static uint8_t types[6] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN, 		// DEBUG
		MAGENTA 	// TRACE
	};

	SetConsoleTextAttribute(console_handle, types[color]);

	OutputDebugStringA(msg);
	uint64_t length = strlen(msg);
	LPDWORD num_written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), msg, (DWORD)length,
				  num_written, 0);
#endif
}

void append_to_log_file(const char *message) {
	if (p_state && p_state->log_handle.is_valid) {
		uint64_t length = string_length(message);
		uint64_t written = 0;

		if (!filesystem_write(&p_state->log_handle, length, message, &written))
			console_write_error("ERROR: writing console.log", LOG_TYPE_ERROR);
	}
}

void report(const char *expr, const char *message,
			const char *file, int32_t line) {
	log_output(LOG_TYPE_FATAL, 
			"ASSERT FAILURE: %s, Message: '%s', on file: %s, line: %d\n",
			expr, message, file, line);
}

b8 log_init(uint64_t *memory_require, void *state) {
	*memory_require = sizeof(log_state_t);
	if (state == 0)
		return true;

	p_state = state;

	if (!filesystem_open("console.log", MODE_WRITE, false,
						 &p_state->log_handle)) {
	  console_write_error("ERROR: unable to open console.log for writing.",
						  LOG_TYPE_ERROR);
	  return false;
	}

	return true;
}

void log_shut(void *state) {
	(void)state;
	p_state = 0;
}

void log_output(log_type_t type, const char *message, ...) {
	const char *type_string[6] = {
		"[FATAL]: ",
		"[ERROR]: ",
		"[WARNING]: ",
		"[INFO]: ",
		"[DEBUG]: ",
		"[TRACE]: "};

	b8 is_error = type < LOG_TYPE_ERROR;

	char buffer[LENGTH];
	memory_zero(buffer, sizeof(buffer));

	va_list p_arg;
	va_start(p_arg, message);
	vsprintf(buffer, message, p_arg);
	va_end(p_arg);

	string_format(buffer, "%s%s\n", type_string[type],buffer);

	if (is_error) {
		console_write_error(buffer, type);
	} else {
		console_write(buffer, type);
	}

	// save file to computer
	append_to_log_file(buffer);
}
