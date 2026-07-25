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

if(APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl)
    pkg_check_modules(SDL_NET REQUIRED IMPORTED_TARGET SDL2_net)

    target_link_libraries(AAServer PRIVATE PkgConfig::SDL PkgConfig::SDL_NET)
    target_compile_definitions(AAServer PRIVATE TARGET_UNIX=1)

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
