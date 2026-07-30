/* Case-correcting forwarding shim, build-only (not a project source
   file). stdafx.h -> targetver.h does #include <SDKDDKVer.h> (mixed
   case, matching what Visual Studio's project wizard generates and what
   real Windows SDKs ship). mingw-w64 provides the same functionality but
   as an all-lowercase sdkddkver.h -- fine on a case-insensitive host
   (real Windows, default macOS) but not found at all when cross-
   compiling from a case-sensitive Linux host (this project's Docker-
   based CI/dev flow). Since Build/Win9x/Makefile already sets
   WINVER/_WIN32_WINNT explicitly on the command line, the real
   sdkddkver.h's actual job (defaulting those) is moot here anyway --
   this just needs to exist so the #include resolves.

   #include_next, not #include: a plain #include <sdkddkver.h> here can
   resolve right back to THIS file (infinite self-include, "nested
   include depth exceeds maximum") whenever the search reaches this
   directory through a case-insensitive path -- e.g. Docker Desktop bind-
   mounting a macOS (APFS, case-insensitive) host directory into the
   Linux container, even though the container's own root filesystem is
   case-sensitive. #include_next skips past this file's own directory in
   the search path, guaranteeing it lands on the real system header
   regardless of the mount's case sensitivity. */
#include_next <sdkddkver.h>
