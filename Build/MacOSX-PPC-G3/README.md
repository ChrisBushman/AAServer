# PowerPC G3 / Mac OS X 10.3 "Panther" platform

A **separate** platform from `Build/MacOSX-PPC/` (Tigerbrew/G4): it builds
AAServer to run on a **PowerBook G3** (PowerPC 750, Mac OS X 10.3.9) as well as
G4/G5, using a Panther-native toolchain (Apple `gcc-4.0` + the 10.3.9 SDK,
`-mcpu=750 -mno-altivec`) so the binary links Panther's own system
libstdc++/libSystem — no Tigerbrew gcc-14 10.4 runtime, no AltiVec. See
`Makefile` for the exact flags.

AAServer links the **G3-native SDL 1.2.15 + SDL_net 1.2.8** built from vendored
source into `~/aa-g3/prefix` (default `G3_PREFIX`), with `@executable_path/lib/`
install names so the bundled libs are found relative to the binary (AAServer is
launched without `DYLD_LIBRARY_PATH`). The full recipe for building that prefix
lives in **AmuletsArmor's** `Build/MacOSX-PPC-G3/README.md` — build it once
there; both repos share the same `~/aa-g3/prefix`.

## Building

```sh
AA_PPC_HOST=aa-tiger ./Build/MacOSX-PPC-G3/remote-build.sh
```

Output: `Build/MacOSX-PPC-G3/build/AAServer` — `otool -hv` shows `ppc750`, 0
AltiVec, and `otool -L` shows `@executable_path/lib/libSDL*.dylib` + Panther
system libs.
