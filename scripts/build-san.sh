#!/bin/bash
#
# Build with AddressSanitizer and UndefinedBehaviorSanitizer.
#
# This is a diagnostic build and is kept isolated from the normal one:
#  - objects go to obj-san/, so sanitized and unsanitized objects never mix;
#  - the result is left at src/dms_san and the runtime `dms` binary is not
#    touched.  Copy it into place yourself if you actually want to run it.
#
# Usage: scripts/build-san.sh [--clean] [make args...]

set -u

cd "$(dirname "$0")/.." || exit 1

MAKE_ARGS=()
for arg in "$@"; do
    if [ "$arg" = "--clean" ]; then
        echo "Cleaning sanitizer build..."
        rm -rf obj-san src/dms_san
    else
        MAKE_ARGS+=("$arg")
    fi
done

# -fsanitize needs to reach both the compiler and the linker.  These are
# appended to the normal flags rather than replacing them, so the sanitizer
# build keeps the full warning profile.  _FORTIFY_SOURCE is turned off because
# it conflicts with ASan's own interceptors.
make -C src \
    OBJDIR=../obj-san \
    EXTRA_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0" \
    EXTRA_LDFLAGS="-fsanitize=address,undefined" \
    "${MAKE_ARGS[@]}" dms_new || exit 1

mv src/dms_new src/dms_san
echo
echo "Sanitizer build written to src/dms_san (runtime 'dms' untouched)."
