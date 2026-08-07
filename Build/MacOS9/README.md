# Mac OS 9 / PowerPC platform (Metrowerks CodeWarrior 8)

Builds **AAServer** (the Amulets & Armor dedicated IPX-over-UDP server) as a
classic **Mac OS 9** (not OS X) PowerPC/CFM **SIOUX console** application that
runs on a real Power Mac / PowerBook under Mac OS 9.x.

Unlike the other platforms here, this one is **not** a gcc/Makefile build —
classic Mac OS has no Unix toolchain, so it uses **Metrowerks CodeWarrior 8**
(`AAServer.mcp`), the MSL C++ runtime, and the SIOUX console. It shares the
`TARGET_UNIX` + `AA_REAL_SDL12` code path with the Unix ports and links a
classic-Mac build of **SDL 1.2** (only `SDL_Init(0)` — no video) + **SDL_net**
(the IPX-over-UDP transport, over **Open Transport**).

Verified on a real PowerBook G4 under Mac OS 9: it builds, links, launches, and
prints `Server started on port 21300`, binding the UDP port via SDL_net/OT.

## Files in this folder

| File | Purpose |
|------|---------|
| `AAServer.mcp` | CodeWarrior 8 project, based on the "Mac OS Classic / Std C++ Console" stationery. Its access paths are machine-local — retarget the source folder, the SDL/SDL_net include paths, and the libs when you open it. |
| `AAServer_OS9_Prefix.h` | CW prefix file (set as the target's Prefix File). Force-includes `<MacHeaders.c>`, sets the platform defines (`TARGET_UNIX` + `AA_REAL_SDL12`, **not** `WIN32` — see below) and the CW pragmas (`ANSI_strict off`, `mpwc_relax on`, `require_prototypes off`, `bool on` for C++), and strips the MSVC calling-convention keywords. |
| `aaserver_os9_compat.c` | Add to the project. One shim: a `pascal SetDialogTimeout` no-op so DialogsLib (whose `MacDialogsLib` import doesn't exist on classic OS 9 → "app is damaged") isn't linked. |

## Building it (from the repo `AAServer/` source dir on the Mac OS 9 machine)

Sources: **`AAServer.cpp` + `PACKETPR.C`** (not `stdafx.cpp` — that's the
Windows PCH). The 13 headers are found next to the sources.

**Libraries to link** (beyond the stationery's MSL C++/SIOUX/InterfaceLib): the
classic-Mac `libSDL` + `SDL_net` (built from the vendored sources, shared with
the game build), `OpenTransportLib` + `OpenTptInternetLib` +
`OpenTransportAppPPC.o` + `OpenTptInetPPC.o` (SDL_net's transport),
`DrawSprocketLib` + `InputSprocketLib` + `CursorDevicesGlue.o` (libSDL link-time
deps, not called at runtime). **Do not** link DialogsLib (see the shim above)
or a second MSL.

**Access paths**: the SDL 1.2 include dir and the SDL_net dir (so `<SDL.h>` /
`<SDL_net.h>` resolve).

Headless builds are driven with **cmdide** (Rebecca Heineman's CodeWarrior
command-line tool, built natively for Tiger — see the `ChrisBushman/cmdide`
fork's `tiger/`): `cmdide -proj -r -b -e -t 1800 AAServer.mcp`.

## Why not `WIN32`

The reference Unix builds (`Build/MacOSX-PPC-G3/Makefile`) define only
`TARGET_UNIX` + `AA_REAL_SDL12`. Defining `WIN32` (as the game's OS 9 build
does) would activate `OPTIONS.H`'s MSVC `#pragma warning(...)` and DOS/Win
keyword stubs; the server doesn't need them, so it stays off — matching the
reference.

## Classic-Mac source fixes (all guarded — no other platform affected)

The wire format needed **no** endianness changes (the big-endian IRIX/OS-X-PPC
builds were already correct). These are CodeWarrior- and classic-Mac-specific:

- **PACK / `GCC_ATTRIBUTE(packed)` are empty on CodeWarrior.** CW has no
  per-field packed attribute, so the on-the-wire packet structs would get PPC
  natural alignment (padding) and mismatch every other platform's client.
  Fixed by wrapping the wire-struct regions in `#pragma options align=packed` /
  `align=reset` (`PACKET.H`, `SYNCPACK.H`, and `ipx.h`'s IPX structs), and
  making `config.h`'s `GCC_ATTRIBUTE` empty on CW (`!defined(__MWERKS__)`).
- **`GENERAL.H` Unix headers.** Its `TARGET_UNIX` block includes
  `<sys/types.h>`/`<strings.h>`/`<unistd.h>`, none of which exist in classic
  MSL; guarded for `macintosh` to include `<time.h>` (for `time_t`) instead.
- **Unix/OS-X process machinery guarded out.** `AAServer.cpp`'s
  `SpawnInTerminal`/`ResolveExecutablePath` (via `readlink("/proc/self/exe")` /
  `_NSGetExecutablePath`) have no classic-Mac equivalent and aren't needed — a
  SIOUX app already owns a console — so they're `#if …!defined(macintosh)`'d out.
- **SDL `main` de-hijack.** Classic SDL 1.2's `SDL_main.h` does
  `#define main SDL_main`; a SIOUX server has no SDLMain, so `AAServer.cpp`
  does `#undef main` (macintosh-guarded) to keep its own entry point.
- **Text-mode fopen** `"a"→"ab"` in `PACKETPR.C` (classic Mac translates line
  endings); the prefix does **not** define `_CRT_SECURE_NO_WARNINGS` because
  `PACKETPR.C` defines it itself (avoids a macro-redefined error).

## Running it

Launch the app; the SIOUX console shows the banner and `Server started on
port <N>` (default 21300). Pass a port number as the first argument to override.
