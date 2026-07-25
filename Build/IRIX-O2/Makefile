# Native Makefile for IRIX 6.5 / SGI O2 (IP32, MIPS R12000).
# Run this ON the O2 (TGCware's GCC 4.5.2 is the toolchain), or via
# remote-build.sh from a modern dev machine over SSH.
# See AmuletsArmor's Build/IRIX-O2/README.md for toolchain/dependency setup
# (SDL 1.2.15 / SDL_net 1.2.8 via Nekoware, at /usr/nekoware).

REPO_ROOT := $(shell cd ../.. && pwd)
SRC_DIR   := $(REPO_ROOT)/AAServer
BUILD_DIR := build

CC         := /usr/tgcware/gcc45/bin/gcc
CXX        := /usr/tgcware/gcc45/bin/g++
SDL_CONFIG := /usr/nekoware/bin/sdl-config

SDL_CFLAGS := $(shell $(SDL_CONFIG) --cflags)
SDL_LIBS   := $(shell $(SDL_CONFIG) --libs) -L/usr/nekoware/lib -lSDL_net

# Real SDL 1.2 (Nekoware), not a compat shim -- see Build/MacOSX-PPC/Makefile
# comment (same reconciliation, same reason).
DEFINES := -DTARGET_UNIX=1 -DAA_REAL_SDL12=1

INCLUDES := -I$(SRC_DIR)

OPT_FLAGS := -g -O2
STD_FLAGS := -std=gnu99
DEPFLAGS  := -MMD -MP

ALL_CFLAGS   := $(OPT_FLAGS) $(STD_FLAGS) $(DEFINES) $(SDL_CFLAGS) $(INCLUDES) $(DEPFLAGS)
# -U__c99: see AmuletsArmor's Build/IRIX-O2/Makefile comment -- g++ 4.5.2 on
# this target predefines __c99, which makes IRIX's <internal/sgimacros.h>
# expand __restrict to the bare C99 keyword, which g++'s C++ frontend
# mis-parses as a parameter name in certain unnamed-parameter prototypes
# pulled in transitively via SDL.h. Removing the built-in sidesteps it.
ALL_CXXFLAGS := $(OPT_FLAGS) $(DEFINES) -U__c99 $(SDL_CFLAGS) $(INCLUDES) $(DEPFLAGS)

TARGET := $(BUILD_DIR)/AAServer

OBJS := $(BUILD_DIR)/AAServer.cpp.o $(BUILD_DIR)/PACKETPR.C.o

.PHONY: all clean run

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link with $(CC) (plain gcc), not $(CXX)/g++ -- see AmuletsArmor's
# Build/IRIX-O2/Makefile comment: linking as C++ pulls in libstdc++, whose
# interaction with pthread static init crashes with SIGSEGV before main()
# on this specific TGCware GCC 4.5.2 / IRIX combination. AAServer.cpp has
# no real C++ runtime features (no new/delete, exceptions, RTTI, STL), so
# nothing here actually needs libstdc++.
$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(SDL_LIBS) -lm

$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.C.o: $(SRC_DIR)/%.C
	$(CC) $(ALL_CFLAGS) -x c -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -rf $(BUILD_DIR)

run: all
	$(REPO_ROOT)/$(TARGET)
