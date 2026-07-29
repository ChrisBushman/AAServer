cmake_minimum_required(VERSION 3.20)
project(AAServer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AAServer
    AAServer/AAServer.cpp
    AAServer/PACKETPR.C
)

set_source_files_properties(AAServer/PACKETPR.C PROPERTIES LANGUAGE C)

target_include_directories(AAServer PRIVATE AAServer)

if(UNIX)
    # CMake's UNIX is true on both Linux and APPLE (macOS/PPC-Tiger are
    # Unix-based); this covers all of them with the one TARGET_UNIX path,
    # matching the AmuletsArmor client build.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl)
    pkg_check_modules(SDL_NET REQUIRED IMPORTED_TARGET SDL2_net)

    target_link_libraries(AAServer PRIVATE PkgConfig::SDL PkgConfig::SDL_NET)
    target_compile_definitions(AAServer PRIVATE TARGET_UNIX=1)
endif()

if(UNIX AND NOT APPLE)
    # Let the release archive be self-contained: a lib/ folder copied in
    # alongside the binary (see .github/workflows/linux-release-build.yml)
    # is found via $ORIGIN-relative rpath, matching the AmuletsArmor
    # client build's own Linux rpath and the existing IRIX-O2/PPC-Tiger
    # pattern of bundling SDL libs instead of requiring them preinstalled.
    # --disable-new-dtags: see the matching comment in AmuletsArmor's own
    # CMakeLists.txt -- forces transitive DT_RPATH instead of GCC's
    # default non-transitive DT_RUNPATH.
    target_link_options(AAServer PRIVATE -Wl,--disable-new-dtags)
    set_target_properties(AAServer PROPERTIES
        BUILD_RPATH "$ORIGIN/lib"
        INSTALL_RPATH "$ORIGIN/lib"
    )
endif()

if(APPLE)
    # sdl12-compat loads SDL2 dynamically; add Homebrew/pkg-config lib dirs to
    # the runtime search path so startup doesn't depend on shell env (same
    # approach as the AmuletsArmor client build).
    set(AA_RPATH_DIRS
        "/usr/local/opt/sdl12-compat/lib"
        "/usr/local/opt/sdl2/lib"
    )
    if(SDL_LIBRARY_DIRS)
        list(APPEND AA_RPATH_DIRS ${SDL_LIBRARY_DIRS})
    endif()
    if(SDL_NET_LIBRARY_DIRS)
        list(APPEND AA_RPATH_DIRS ${SDL_NET_LIBRARY_DIRS})
    endif()
    list(REMOVE_DUPLICATES AA_RPATH_DIRS)

    set_target_properties(AAServer PROPERTIES
        BUILD_RPATH "${AA_RPATH_DIRS}"
        INSTALL_RPATH "${AA_RPATH_DIRS}"
    )
endif()
