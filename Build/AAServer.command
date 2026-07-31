#!/bin/sh
# Finder recognizes the .command extension and opens this in a new
# Terminal.app window automatically -- unlike the bare AAServer binary,
# which Finder has no reliable way to run with a visible window. Run
# from a shell/SSH session directly with ./AAServer instead if you don't
# want a new terminal spawned (add --console yourself if you do).
cd "$(dirname "$0")" || exit 1
DYLD_LIBRARY_PATH="$(pwd)/lib:$DYLD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH
exec ./AAServer "$@"
