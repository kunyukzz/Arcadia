#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "engine/define.h"

typedef struct file_handle_t {
	void *handle;
	b8 is_valid;
} file_handle_t;

typedef enum file_mode_t {
	MODE_READ = 0x1,
	MODE_WRITE = 0x2
} file_mode_t;

b8 filesystem_exists(const char* path);
b8 filesystem_open(const char* path, file_mode_t mode, b8 binary, file_handle_t* handle);
void filesystem_close(file_handle_t* handle);

b8 filesystem_read_line(file_handle_t* handle, char** line_buff);
b8 filesystem_write_line(file_handle_t* handle, const char* text);

b8 filesystem_read(file_handle_t* handle, uint64_t data_size, void* out_data, uint64_t* out_byte_read);
b8 filesystem_read_all_byte(file_handle_t* handle, uint8_t** out_bytes, uint64_t* out_byte_read);

b8 filesystem_write(file_handle_t* handle, uint64_t data_size, const void* data, uint64_t* out_byte_written);

#endif //__FILESYSTEM_H__
