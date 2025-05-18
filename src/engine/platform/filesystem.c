#include "engine/platform/filesystem.h"
#include "engine/core/logger.h"
//#include "engine/memory/memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

b8 filesystem_exists(const char* path) {
	struct stat buffer;
	return stat(path, &buffer) == 0;
}

b8 filesystem_open(const char* path, file_mode_t mode, b8 binary, file_handle_t* handle) {
	handle->is_valid = false;
	handle->handle = 0;
	const char *mode_str;

	if ((mode & MODE_READ) != 0 && (mode & MODE_WRITE) != 0) {
		mode_str = binary ? "w+b" : "w+";
	} else if ((mode & MODE_READ) != 0 && (mode & MODE_WRITE) == 0) {
		mode_str = binary ? "rb" : "r";
	} else if ((mode & MODE_READ) == 0 && (mode & MODE_WRITE) != 0) {
		mode_str = binary ? "wb" : "w";
	} else {
		ar_ERROR("Invalid mode try to open file: '%s'", path);
		return false;
	}

	FILE *file = fopen(path, mode_str);
	if (!file) {
		ar_ERROR("Error open file: '%s'", path);
		return false;
	}

	handle->handle = file;
	handle->is_valid = true;
	return true;
}

void filesystem_close(file_handle_t* handle) {
	if (handle->handle) {
		fclose((FILE *)handle->handle);
		handle->handle = 0;
		handle->is_valid = false;
	}
}

b8 filesystem_read_line(file_handle_t *handle, uint64_t max, char **line_buff,
                        uint64_t *line_length) {
    if (handle->handle && line_buff && line_length && max > 0) {
        char *buffer = *line_buff;

        if (fgets(buffer, max, (FILE *)handle->handle) != 0) {
            *line_length = strlen(*line_buff);
            return true;
        }
    }

    return false;
}

b8 filesystem_write_line(file_handle_t* handle, const char* text) {
	if (handle->handle) {
		int32_t result = fputs(text, (FILE *)handle->handle);

		if (result != EOF)
			result  = fputc('\n', (FILE *)handle->handle);

		fflush((FILE *)handle->handle);
		return result != EOF;
	}

	return false;
}

b8 filesystem_read(file_handle_t *handle, uint64_t data_size, void *out_data,
                   uint64_t *out_byte_read) {
    if (handle->handle && out_data) {
        *out_byte_read = fread(out_data, 1, data_size, (FILE *)handle->handle);

        if (*out_byte_read != data_size)
            return false;

        return true;
    }
    return false;
}

b8 filesystem_read_all_byte(file_handle_t *handle, uint8_t *out_bytes,
                            uint64_t *out_byte_read) {
    if (handle->handle && out_bytes && out_byte_read) {
        uint64_t size = 0;
        if (!filesystem_size(handle, &size)) {
            return false;
        }

        *out_byte_read = fread(out_bytes, 1, size, (FILE *)handle->handle);
        return *out_byte_read == size;
    }
    return false;
}

b8 filesystem_read_all_text(file_handle_t *handle, char *text,
                            uint64_t *out_byte_read) {
    if (handle->handle && text && out_byte_read) {
        uint64_t size = 0;
        if (!filesystem_size(handle, &size)) {
            return false;
        }

        *out_byte_read = fread(text, 1, size, (FILE *)handle->handle);
        return *out_byte_read == size;
    }

    return false;
}

b8 filesystem_write(file_handle_t *handle, uint64_t data_size, const void *data,
                    uint64_t *out_byte_written) {
    if (handle->handle) {
        *out_byte_written = fwrite(data, 1, data_size, (FILE *)handle->handle);

        if (*out_byte_written != data_size)
            return false;

        fflush((FILE *)handle->handle);
        return true;
    }
    return false;
}

b8 filesystem_size(file_handle_t *handle, uint64_t *size) {
 	if (handle->handle) {
		fseek((FILE *)handle->handle, 0, SEEK_END);
		*size = (uint64_t)ftell((FILE *)handle->handle);
		rewind((FILE *)handle->handle);
		return true;
	}

	return false;
}
