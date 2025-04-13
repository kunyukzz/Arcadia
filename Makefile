CC = clang
CFLAGS = -g -std=c99 -I src -Wall -Wextra -Wno-c23-extensions 				\
		 -Wshadow -Wpointer-arith -Wcast-align -Wsign-conversion 			\
		 -Wno-error=uninitialized -fsanitize=address -fno-omit-frame-pointer
INCLUDE = -I$(VULKAN_SDK)/include
SRC = $(shell find src -name '*.c')
OBJ_DIR = obj
OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC))
OUT = bin/rcadia

# Detect OS
OS := $(shell uname)

# Default: Linux settings
LDFLAGS = -lasan -lvulkan -lxcb -lX11 -lX11-xcb -lrt -L$(VULKAN_SDK)/lib

# Windows settings (Modify if using MinGW or MSVC)
ifeq ($(OS), Windows_NT)
    CC = clang
    CFLAGS += -DWIN32
    LDFLAGS = -lopengl32 -lgdi32
endif

DIR = $(sort $(dir $(OBJ)))

all: $(OUT)

$(OUT): $(OBJ)
	@mkdir -p bin
	@$(CC) $(OBJ) -o $(OUT) $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c | create_dir
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $(INCLUDE) $< -o $@

create_dir:
	@mkdir -p $(DIR)

clean:
	@echo "Clean projects artifacts...$<"
	@rm -f $(shell find -name '*.o') $(OBJ) $(OUT)

.PHONY: all clean
