# Cross-compiled Makefile for Windows 95/98/ME (i686, real native Windows
# build -- not TARGET_UNIX). See the sibling AmuletsArmor repo's
# Build/Win9x/Makefile for the full toolchain rationale (Ubuntu's
# gcc-mingw-w64-i686, not Homebrew's UCRT-locked one) and
# SDL_net-1.2.8-src/README-win9x.md for why SDL_net needs to be rebuilt
# from source. Run on Linux/macOS with a mingw-w64 cross toolchain; there
# is no way to build (or run) this target on the box it's actually for.

REPO_ROOT       := $(shell cd ../.. && pwd)
SRV_DIR         := $(REPO_ROOT)/AAServer
SDL_SRC_DIR     := $(REPO_ROOT)/SDL-1.2.15-src
SDL_WINMAIN_DIR := $(REPO_ROOT)/SDL-1.2.15-winmain
SDLNET_SRC_DIR  := $(REPO_ROOT)/SDL_net-1.2.8-src
SHIM_DIR        := $(REPO_ROOT)/Build/Win9x/mingw-shims

BUILD_DIR := build

CC  := i686-w64-mingw32-gcc
CXX := i686-w64-mingw32-g++

# See AmuletsArmor's Build/Win9x/Makefile for why 0x0410/0x0400/0x0410,
# __USE_MINGW_ANSI_STDIO=0 (confirmed real-hardware fix for a documented
# mingw-w64 bug: MSVCRT.DLL:_strtoui64 doesn't exist on 9x), and
# -march=i486/-mtune=i486 (THE fix for a second real-hardware bug:
# i686-w64-mingw32-gcc defaults to -march=pentiumpro with no explicit
# -march, silently emitting CMOV and other P6-only instructions absent on
# 486/Pentium/Pentium MMX -- confirmed by all mingw-built binaries here
# crashing on real Windows 95 hardware with "invalid instruction" at
# identical bytes, including AALauncher.exe, which shares no code with
# AAServer/AA at all).
WIN9X_VER_DEFINES := -DWINVER=0x0410 -D_WIN32_WINNT=0x0400 -D_WIN32_WINDOWS=0x0410 \
                      -D__USE_MINGW_ANSI_STDIO=0 -fno-stack-protector \
                      -march=i486 -mtune=i486

# SHIM_DIR first (SDKDDKVer.h case-correcting forward, see that file's
# own comment); SDLNET_SRC_DIR before SRV_DIR: this repo also ships a
# vendored SDL_net-1.2.8/include with the real headers, but SRV_DIR
# itself has no shim to shadow -- kept in this order for consistency
# with AmuletsArmor's own Makefile and to make sure the *patched* source
# copy's SDL_net.h is what's found, not an unrelated one.
INCLUDES := -I$(SHIM_DIR) -I$(SDLNET_SRC_DIR) -I$(SRV_DIR) -I$(SDL_SRC_DIR)/include

DEFINES := $(WIN9X_VER_DEFINES) -D_CRT_SECURE_NO_WARNINGS=1 -Dcdecl=

OPT_FLAGS := -O2
DEPFLAGS  := -MMD -MP

ALL_CXXFLAGS := $(OPT_FLAGS) $(DEFINES) $(INCLUDES) $(DEPFLAGS)

TARGET := $(BUILD_DIR)/AAServer.exe

SDL_IMPLIB    := $(BUILD_DIR)/libSDL.dll.a
SDLNET_IMPLIB := $(BUILD_DIR)/libSDL_net.dll.a
SDLNET_DLL    := $(BUILD_DIR)/SDL_net.dll

OBJS := $(BUILD_DIR)/AAServer.cpp.o $(BUILD_DIR)/PACKETPR.C.o

.PHONY: all clean package

all: $(BUILD_DIR) $(TARGET) $(BUILD_DIR)/SDL.dll

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# SDL: built from vendored source, not the prebuilt DLL -- real Windows
# 95 hardware crashed *inside* the prebuilt SDL.dll. See the sibling
# AmuletsArmor repo's SDL-1.2.15-src/README-win9x.md for the full story
# (including the _strtoui64 root cause forced out via the ac_cv_func_*
# overrides below).
$(SDL_SRC_DIR)/build/.libs/SDL.dll $(SDL_SRC_DIR)/build/.libs/libSDL.dll.a &:
	cd $(SDL_SRC_DIR) && \
	CC=i686-w64-mingw32-gcc CFLAGS="$(OPT_FLAGS) $(WIN9X_VER_DEFINES)" \
	./configure --host=i686-w64-mingw32 --enable-shared --disable-static \
	  ac_cv_func_strtoll=no ac_cv_func_strtoull=no \
	  ac_cv_func__strtoll=no ac_cv_func__strtoull=no && \
	$(MAKE) -j$$(nproc)

$(BUILD_DIR)/SDL.dll: $(SDL_SRC_DIR)/build/.libs/SDL.dll | $(BUILD_DIR)
	cp $< $@

$(SDL_IMPLIB): $(SDL_SRC_DIR)/build/.libs/libSDL.dll.a | $(BUILD_DIR)
	cp $< $@

SDLNET_OBJS := $(addprefix $(BUILD_DIR)/sdlnet-,$(notdir $(patsubst %.c,%.o,$(wildcard $(SDLNET_SRC_DIR)/*.c))))

$(BUILD_DIR)/sdlnet-%.o: $(SDLNET_SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(OPT_FLAGS) $(WIN9X_VER_DEFINES) -I$(SDL_SRC_DIR)/include -I$(SDLNET_SRC_DIR) $(DEPFLAGS) -c $< -o $@

$(SDLNET_DLL) $(SDLNET_IMPLIB) &: $(SDLNET_OBJS) $(SDL_IMPLIB)
	$(CC) -shared -o $(SDLNET_DLL) $(SDLNET_OBJS) $(SDL_IMPLIB) -lws2_32 -Wl,--out-implib,$(SDLNET_IMPLIB)

# NO_STDIO_REDIRECT: AAServer is a real console app (status/log printfs
# to the console, confirmed already-working behavior on every other
# platform) -- vanilla SDL_win32_main.c's WinMain() otherwise redirects
# stdout/stderr to stdout.txt/stderr.txt (the right default for a
# windowed app with no console, wrong here).
$(BUILD_DIR)/SDL_win32_main.c.o: $(SDL_WINMAIN_DIR)/SDL_win32_main.c | $(BUILD_DIR)
	$(CC) $(OPT_FLAGS) $(WIN9X_VER_DEFINES) -DNO_STDIO_REDIRECT -I$(SDL_SRC_DIR)/include $(DEPFLAGS) -c $< -o $@

# AAServer.cpp's real-Windows branch names its entry point _tmain (see
# <tchar.h>; _MBCS, not _UNICODE, so this expands to plain main/char*)
# -- but including SDL.h still triggers SDL_main.h's #define main
# SDL_main, same as the AmuletsArmor client, since mingw always defines
# __WIN32__ for a real Windows target regardless of anything this
# Makefile does. Needs the same WinMain() wrapper for the same reason.
$(TARGET): $(OBJS) $(BUILD_DIR)/SDL_win32_main.c.o $(SDL_IMPLIB) $(SDLNET_IMPLIB)
	$(CC) -o $@ $(OBJS) $(BUILD_DIR)/SDL_win32_main.c.o $(SDL_IMPLIB) $(SDLNET_IMPLIB) -mconsole

$(BUILD_DIR)/AAServer.cpp.o: $(SRV_DIR)/AAServer.cpp | $(BUILD_DIR)
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/PACKETPR.C.o: $(SRV_DIR)/PACKETPR.C | $(BUILD_DIR)
	$(CC) $(OPT_FLAGS) $(DEFINES) $(INCLUDES) $(DEPFLAGS) -x c -c $< -o $@

-include $(OBJS:.o=.d)
-include $(SDLNET_OBJS:.o=.d)
-include $(BUILD_DIR)/SDL_win32_main.c.d

clean:
	rm -rf $(BUILD_DIR)

package: all $(SDLNET_DLL)
	mkdir -p $(BUILD_DIR)/package
	cp $(TARGET) $(BUILD_DIR)/SDL.dll $(SDLNET_DLL) $(BUILD_DIR)/package/
