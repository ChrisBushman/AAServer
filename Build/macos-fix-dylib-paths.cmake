# Post-build step for AAServer's modern-macOS (APPLE, CMake) target.
#
# The executable links its SDL dependencies (sdl12-compat's libSDL, its
# own libSDL2 re-export, and libSDL2_net) as hardcoded absolute Homebrew
# paths -- e.g. /usr/local/opt/sdl12-compat/lib/libSDL-1.2.0.dylib on
# Intel Homebrew, or /opt/homebrew/opt/... on Apple Silicon. Those
# absolute paths make the release archive non-portable: DYLD_LIBRARY_PATH
# (what the run.sh/.command wrappers set) is only consulted by dyld for
# bare/relative dylib names, never for an already-absolute LC_LOAD_DYLIB
# path, so a binary built on one Mac refuses to launch on any machine
# without that exact Homebrew prefix present.
#
# This rewrites each such dependency to @executable_path/lib/<name>
# (matching how the Linux/PPC/IRIX builds already bundle their own SDL
# libs via a relative lib/ folder) and copies the real .dylib alongside
# the binary so @executable_path/lib actually resolves. Re-signs
# afterward (ad hoc) since install_name_tool invalidates the existing
# code signature, and an unsigned binary won't launch on Apple Silicon.
#
# Invoked as:
#   cmake -DTARGET_FILE=... -DOTOOL=... -DINSTALL_NAME_TOOL=...
#         -DCODESIGN=... -P this-file

execute_process(
    COMMAND ${OTOOL} -L ${TARGET_FILE}
    OUTPUT_VARIABLE OTOOL_OUTPUT
)
string(REPLACE "\n" ";" OTOOL_LINES "${OTOOL_OUTPUT}")

get_filename_component(TARGET_DIR "${TARGET_FILE}" DIRECTORY)
set(LIB_DIR "${TARGET_DIR}/lib")
file(MAKE_DIRECTORY "${LIB_DIR}")

set(REWROTE_ANY FALSE)
foreach(LINE ${OTOOL_LINES})
    string(STRIP "${LINE}" LINE)
    if(LINE MATCHES "^((/usr/local/opt|/opt/homebrew/opt)/[^ ]+\\.dylib)")
        set(OLD_PATH "${CMAKE_MATCH_1}")
        get_filename_component(LIB_NAME "${OLD_PATH}" NAME)
        message(STATUS "AAServer/macOS: rewriting ${OLD_PATH} -> @executable_path/lib/${LIB_NAME}")
        execute_process(COMMAND ${INSTALL_NAME_TOOL}
            -change "${OLD_PATH}" "@executable_path/lib/${LIB_NAME}"
            "${TARGET_FILE}")
        file(COPY "${OLD_PATH}" DESTINATION "${LIB_DIR}")
        set(REWROTE_ANY TRUE)
    endif()
endforeach()

if(REWROTE_ANY)
    execute_process(COMMAND ${CODESIGN} --force -s - "${TARGET_FILE}")
endif()
