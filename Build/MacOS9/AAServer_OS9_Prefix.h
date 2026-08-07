/* AAServer_OS9_Prefix.h -- CodeWarrior target prefix file for the Mac OS 9
 * (PPC/CFM) AAServer build. Set this as the target's "Prefix File" (C/C++
 * Language panel). It force-includes the classic Mac Toolbox headers, then
 * injects the platform routing + build knobs.
 *
 * AAServer is a headless SIOUX console server: SDL only for timing/SDL_Init(0)
 * (no video) + SDL_net for the IPX-over-UDP transport. The reference Mac build
 * (Build/MacOSX-PPC-G3/Makefile) defines only TARGET_UNIX + AA_REAL_SDL12, so
 * this mirrors that (no WIN32 -- PACKETPR.C's `#if WIN32` must fall to the
 * fopen("packets.txt") branch, not Windows stdout).
 */
#include <MacHeaders.c>          /* classic Mac PPC Toolbox, precompiled */

/* AAServer.cpp is C++, PACKETPR.C is C (uppercase .C). Both use // comments;
   allow them (and non-int bitfields). */
#pragma ANSI_strict off

/* T_byte8* (unsigned char*) string buffers vs. char* literals/APIs: CW makes
   that a hard "illegal implicit conversion" error unless MPW-relaxed pointer
   rules are on. (= the "Relaxed Pointer Type Rules" panel checkbox.) */
#pragma mpwc_relax on
#pragma warn_implicitconv off

/* Don't make a missing prototype a hard error. */
#pragma require_prototypes off

/* AAServer.cpp uses `bool`; the panel's "Enable bool Support" is off, so turn
   the keyword on for C++ translation units. */
#ifdef __cplusplus
#pragma bool on
#endif

/* --- platform routing -------------------------------------------------- */
#define TARGET_UNIX   1          /* route timing/networking through SDL/SDL_net,
                                    not Win32; also selects main() over _tmain */
#define AA_REAL_SDL12 1          /* link a genuine SDL 1.2 (bare <SDL.h>/<SDL_net.h>
                                    paths), like the G3/Panther, MacOSX-PPC and
                                    IRIX-O2 Makefile builds -- not sdl12-compat/SDL2 */

/* --- build knobs ------------------------------------------------------- */
#define NDEBUG        1          /* release semantics (DEBUG.H DebugRoutine -> no-ops) */
#define NO_ASSEMBLY   1
/* _CRT_SECURE_NO_WARNINGS is deliberately NOT defined here -- it is an MSVC
   no-op on CodeWarrior, and PACKETPR.C #defines it itself (empty); defining it
   =1 in this force-included prefix makes that a "macro redefined" error. */
#define _MBCS         1

/* --- strip MSVC calling-convention keywords ---------------------------- */
#define cdecl
#define _cdecl
#define __cdecl
#define __fastcall
#define __stdcall
