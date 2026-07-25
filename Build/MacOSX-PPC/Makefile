# Native Makefile for PowerPC / Mac OS X 10.4 "Tiger".
# Run this ON the Tiger box (Tigerbrew's gcc is the primary supported
# toolchain), or via remote-build.sh from a modern dev machine over SSH.
# See AmuletsArmor's Build/MacOSX-PPC/README.md for toolchain/dependency
# setup (SDL 1.2.15 / SDL_net 1.2.8 via Tigerbrew).

REPO_ROOT := $(abspath ../..)
SRC_DIR   := $(REPO_ROOT)/AAServer
BUILD_DIR := build

CC          ?= gcc
CXX         ?= g++
ARCH        ?= ppc
MACOSX_VERSION_MIN ?= 10.4
SDKROOT     ?=
SDL_CONFIG  ?= sdl-config

SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
SDL_LIBS    := $(shell $(SDL_CONFIG) --libs) -lSDL_net

# Real SDL 1.2 (Tigerbrew), not the modern-macOS sdl12-compat + SDL2 combo --
# AAServer.cpp/ipx.h need to know which, since real SDL_net headers live at
# a bare path, not under SDL2/.
DEFINES := -DTARGET_UNIX=1 -DAA_REAL_SDL12=1

ARCHFLAGS := -arch $(ARCH) -mmacosx-version-min=$(MACOSX_VERSION_MIN)
ifneq ($(strip $(SDKROOT)),)
ARCHFLAGS += -isysroot $(SDKROOT)
endif

INCLUDES := -I$(SRC_DIR)

OPT_FLAGS := -g -O2
STD_FLAGS := -std=gnu99
DEPFLAGS  := -MMD -MP

ALL_CFLAGS   := $(ARCHFLAGS) $(OPT_FLAGS) $(STD_FLAGS) $(DEFINES) $(SDL_CFLAGS) $(INCLUDES) $(DEPFLAGS)
ALL_CXXFLAGS := $(ARCHFLAGS) $(OPT_FLAGS) $(DEFINES) $(SDL_CFLAGS) $(INCLUDES) $(DEPFLAGS)

TARGET := $(BUILD_DIR)/AAServer

OBJS := $(BUILD_DIR)/AAServer.cpp.o $(BUILD_DIR)/PACKETPR.C.o

.PHONY: all clean run

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CXX) $(ARCHFLAGS) -o $@ $(OBJS) $(SDL_LIBS)

$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.C.o: $(SRC_DIR)/%.C
	$(CC) $(ALL_CFLAGS) -x c -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -rf $(BUILD_DIR)

run: all
	$(REPO_ROOT)/$(TARGET)
