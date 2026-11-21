CC := gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c11
DEBUG_CFLAGS ?= -Wall -Wextra -g -O0 -std=c11 -DDEBUG

SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build

# OS Detection
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Linux)
    PLATFORM := linux
    EXE :=
    PKGS := webkitgtk-6.0 gtk4
    PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
    PKG_LIBS := $(shell pkg-config --libs $(PKGS))
    COPY_DEPS :=
else ifeq ($(UNAME_S), Darwin)
    PLATFORM := macos
    EXE :=
    PKG_CFLAGS :=
    PKG_LIBS := -framework Cocoa -framework WebKit
    COPY_DEPS :=
else
    PLATFORM := windows
    EXE := .exe
    PKG_CFLAGS :=
    PKG_LIBS := -luser32 -lgdi32 -lole32 -lshlwapi
    COPY_DEPS := @test -f lib/WebView2Loader.dll && cp -f lib/WebView2Loader.dll $(BUILD_DIR)/ || true
endif

TARGET := $(BUILD_DIR)/app$(EXE)
EMBED_BIN := $(BUILD_DIR)/embed$(EXE)

# Common source files in src/ (app.c, ipc.c, memi.c, main.c)
COMMON_SRCS := $(wildcard $(SRC_DIR)/*.c)
PLATFORM_SRCS := $(wildcard $(SRC_DIR)/platform/$(PLATFORM)/*.c) $(wildcard $(SRC_DIR)/platform/$(PLATFORM)/*.m)
SRCS := $(COMMON_SRCS) $(PLATFORM_SRCS)

# Convert src/foo.c -> build/foo.o and src/platform/... -> build/platform/...
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter %.c, $(SRCS))) \
        $(patsubst $(SRC_DIR)/%.m, $(BUILD_DIR)/%.o, $(filter %.m, $(SRCS)))

# ANSI colors for clean output
CYAN   := \033[36m
GREEN  := \033[32m
YELLOW := \033[33m
RESET  := \033[0m

.PHONY: all run debug clean

all: $(TARGET)

# 1. Compile the self-hosted embed tool
$(EMBED_BIN): tools/embed.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $< -o $@

# 2. Generate bridge.h whenever assets/bridge.js changes
$(BUILD_DIR)/bridge.h: assets/bridge.js $(EMBED_BIN) | $(BUILD_DIR)
	@./$(EMBED_BIN) assets/bridge.js $@ bridge_js

$(TARGET): $(OBJS) | $(BUILD_DIR)
	@printf "  $(GREEN)[LD]$(RESET)    $@\n"
	@$(CC) $(OBJS) $(PKG_LIBS) -o $@
	$(COPY_DEPS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/bridge.h | $(BUILD_DIR)
	@mkdir -p $(dir $@)
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
