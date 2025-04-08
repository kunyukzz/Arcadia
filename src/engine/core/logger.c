#include "logger.h"
#include "assertion.h"

#include "define.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

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

void console_write(const char *msg, uint8_t color) {
#if OS_LINUX
    const char *color_string[] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN 		// DEBUG
	};

    printf("\033[%sm%s\033[0m", color_string[color], msg);

#elif OS_WINDOWS
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	static uint8_t types[5] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN 		// DEBUG
	};

	SetConsoleTextAttribute(console_handle, types[color]);

	OutputDebugStringA(msg);
	uint64_t length = strlen(msg);
	LPDWORD num_written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), msg, (DWORD)length, num_written, 0);
	fflush(stdout);
#endif
}

void console_write_error(const char *msg, uint8_t color) {
#if OS_LINUX
    const char *color_string[] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN 		// DEBUG
	};

    printf("\033[%sm%s\033[0m", color_string[color], msg);

#elif OS_WINDOWS
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	static uint8_t types[5] = {
		RED, 		// FATAL
		ORANGE, 	// ERROR
		YELLOW, 	// WARNING
		WHITE, 		// INFO
		CYAN 		// DEBUG
	};

	SetConsoleTextAttribute(console_handle, types[color]);

	OutputDebugStringA(msg);
	uint64_t length = strlen(msg);
	LPDWORD num_written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), msg, (DWORD)length, num_written, 0);
	fflush(stdout);
#endif
}

void report(const char *expr, const char *message, const char *file, int32_t line) {
	log_output(LOG_TYPE_FATAL, "ASSERT FAILURE: %s, Message: '%s', on file: %s, line: %d\n", expr, message, file, line);
}

void log_output(log_type_t type, const char *message, ...) {
	const char *type_string[5] = {
		"[FATAL]: ",
		"[ERROR]: ",
		"[WARNING]: ",
		"[INFO]: ",
		"[DEBUG]: "};

	b8 is_error = type < LOG_TYPE_ERROR;

	char buffer[LENGTH];
	memset(buffer, 0, sizeof(buffer));

	va_list p_arg;
	va_start(p_arg, message);
	vsprintf(buffer, message, p_arg);
	va_end(p_arg);

	char final[LENGTH];
	sprintf(final, "%s%s\n", type_string[type], buffer);

	if (is_error) {
		console_write_error(final, type);
	} else {
		console_write(final, type);
	}
}
