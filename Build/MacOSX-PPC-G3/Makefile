# Native Makefile for PowerPC G3 / Mac OS X 10.3 "Panther".
#
# Separate G3 platform (distinct from Build/MacOSX-PPC, which targets a G4
# Tiger box with Tigerbrew SDL). It pins a Panther-native toolchain -- Apple's
# own gcc 4.0 against the 10.3.9 SDK, generic-G3 codegen (-mcpu=750
# -mno-altivec) -- so the binary links Panther's system libSystem/libstdc++
# (current version 71.x), NOT Tigerbrew gcc-14's 10.4 C++ runtime (whose
# $UNIX2003/xlocale imports and AltiVec don't exist / can't run on a G3), and
# links a G3-native SDL/SDL_net built from source with the same toolchain
# (see Build/MacOSX-PPC-G3/README.md; default prefix ~/aa-g3/prefix).
#
# Build ON the Tiger box (it carries /Developer/SDKs/MacOSX10.3.9.sdk and Apple
# gcc 4.0), or via remote-build.sh.

REPO_ROOT := $(abspath ../..)
SRC_DIR   := $(REPO_ROOT)/AAServer
BUILD_DIR := build

# := not ?=: CC/CXX are GNU-make *built-in* variables (cc / g++), so ?= would
# never override them and the build would silently use Tigerbrew gcc-14 (=g++ on
# PATH) instead of Apple gcc 4.0. := beats the built-in default but still loses
# to a command-line `make CC=...` override.
CC          := /usr/bin/gcc-4.0
CXX         := /usr/bin/g++-4.0
ARCH        ?= ppc
MACOSX_VERSION_MIN ?= 10.3
SDKROOT     ?= /Developer/SDKs/MacOSX10.3.9.sdk
G3FLAGS     ?= -mcpu=750 -mno-altivec

# G3-native SDL 1.2.15 + SDL_net 1.2.8, built from the vendored source with
# this same toolchain (AltiVec-free, Panther-linked). Point SDL_CONFIG at that
# prefix's sdl-config, NOT Tigerbrew's on PATH.
G3_PREFIX   ?= $(HOME)/aa-g3/prefix
SDL_CONFIG  ?= $(G3_PREFIX)/bin/sdl-config

SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
SDL_LIBS    := $(shell $(SDL_CONFIG) --libs) -lSDL_net

DEFINES := -DTARGET_UNIX=1 -DAA_REAL_SDL12=1

ARCHFLAGS := -arch $(ARCH) -mmacosx-version-min=$(MACOSX_VERSION_MIN) $(G3FLAGS)
ifneq ($(strip $(SDKROOT)),)
ARCHFLAGS += -isysroot $(SDKROOT)
endif

INCLUDES := -I$(SRC_DIR)

OPT_FLAGS := -g -O2
STD_FLAGS := -std=gnu99
DEPFLAGS  := -MMD -MP

# -fpermissive on C++ only: the 10.3 SDK declares a few libSystem prototypes
# with pre-Tiger signatures (e.g. _NSGetExecutablePath's bufsize is
# `unsigned long *` on 10.3 vs `uint32_t *` on 10.4). The AA sources are
# written to the Tiger signatures; on 32-bit PPC the types are the same width,
# so the mismatch is benign -- C treats it as a warning, but C++ makes it a hard
# error, which -fpermissive downgrades back to a warning. Scoped to this G3
# platform; the Tiger build (10.4 SDK) doesn't need it.
ALL_CFLAGS   := $(ARCHFLAGS) $(OPT_FLAGS) $(STD_FLAGS) $(DEFINES) $(SDL_CFLAGS) $(INCLUDES) $(DEPFLAGS)
ALL_CXXFLAGS := $(ARCHFLAGS) $(OPT_FLAGS) -fpermissive $(DEFINES) $(SDL_CFLAGS) $(INCLUDES) $(DEPFLAGS)

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
