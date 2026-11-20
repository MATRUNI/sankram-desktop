CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c11
DEBUG_CFLAGS ?= -Wall -Wextra -g -O0 -std=c11 -DDEBUG

PKGS := webkitgtk-6.0 gtk4
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS := $(shell pkg-config --libs $(PKGS))

SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
TARGET := $(BUILD_DIR)/app

# Find all .c source files in src/
SRCS := $(wildcard $(SRC_DIR)/*.c)
# Convert src/foo.c -> build/foo.o
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# ANSI colors for clean output
CYAN   := \033[36m
GREEN  := \033[32m
YELLOW := \033[33m
RESET  := \033[0m

.PHONY: all run debug clean

all: $(TARGET)

# 1. Compile the self-hosted embed tool
$(BUILD_DIR)/embed: tools/embed.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $< -o $@

# 2. Generate bridge.h whenever assets/bridge.js changes
$(BUILD_DIR)/bridge.h: assets/bridge.js $(BUILD_DIR)/embed | $(BUILD_DIR)
	@./$(BUILD_DIR)/embed assets/bridge.js $@ bridge_js

$(TARGET): $(OBJS) | $(BUILD_DIR)
	@printf "  $(GREEN)[LD]$(RESET)    $@\n"
	@$(CC) $(OBJS) $(PKG_LIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/bridge.h | $(BUILD_DIR)
	@printf "  $(CYAN)[CC]$(RESET)    $<\n"
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -I$(BUILD_DIR) $(PKG_CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Quick run command
run: $(TARGET)
	@printf "  $(YELLOW)[RUN]$(RESET)   $(TARGET)\n"
	@./$(TARGET)

# Debug build with symbols (-g) and no optimizations (-O0)
debug: CFLAGS := $(DEBUG_CFLAGS)
debug: clean $(TARGET)

clean:
	@printf "  $(YELLOW)[CLEAN]$(RESET) Removing $(BUILD_DIR)/\n"
	@rm -rf $(BUILD_DIR)
