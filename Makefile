CC = clang
STD = -std=c99
INCLUDES = -I src -I$(VULKAN_SDK)/include
WARNINGS = -Wall -Wextra -Wno-c2x-extensions -Wshadow -Wpointer-arith 			\
		   -Wcast-align -Wsign-conversion -Wno-error=uninitialized
MEMORYS = -fsanitize=address -fno-omit-frame-pointer

BUILD ?= debug

ifeq ($(BUILD), debug)
	CFLAGS = -D_DEBUG -g $(STD) $(INCLUDES) $(WARNINGS) 
	LDFLAGS = -lvulkan -lxcb -lX11 -lX11-xcb -lxcb-randr -lrt -lm -L$(VULKAN_SDK)/lib
	OUT_DIR = bin
	OBJ_DIR = obj/debug
else ifeq ($(BUILD), release)
	CFLAGS = -o2 $(STD) $(INCLUDES) $(WARNINGS)
	LDFLAGS = -lvulkan -lxcb -lX11 -lX11-xcb -lxcb-randr -lrt -lm -L$(VULKAN_SDK)/lib
	OUT_DIR = bin
	OBJ_DIR = obj/release
endif

SRC = $(shell find src -type f -name '*.c')
OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRC))
DEP = $(OBJ:.o=.d)
DIR = $(sort $(dir $(OBJ)))
OUT = $(OUT_DIR)/rcadia

-include $(DEP)

# Detect OS\
OS := $(shell uname)
ifeq ($(OS), Windows_NT)
    CFLAGS += -DWIN32
    LDFLAGS = -lopengl32 -lgdi32
endif

all: $(OUT)

$(OUT): $(OBJ)
	@mkdir -p $(OUT_DIR)
	@$(CC) $(OBJ) -o $(OUT) $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c | create_dir
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

create_dir:
	@mkdir -p $(DIR)

clean:
	@echo "Clean projects artifacts..."
	@rm -rf $(OBJ_DIR)
	@rm -f $(OUT)

.PHONY: clean
