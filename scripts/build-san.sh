#!/bin/bash
#
# Build with AddressSanitizer and UndefinedBehaviorSanitizer.
#
# This is a diagnostic build and is kept isolated from the normal one:
#  - objects go to bin/objects/server-san/, so sanitized and unsanitized
#    objects never mix;
#  - the result is left at bin/server/dms_san and the runtime binary is not
#    touched.  Copy it into place yourself if you actually want to run it.
#
# Usage: scripts/build-san.sh [--clean] [make args...]

set -u

cd "$(dirname "$0")/.." || exit 1

SAN_OBJDIR=../bin/objects/server-san
SAN_BINARY=../bin/server/dms_san

MAKE_ARGS=()
for arg in "$@"; do
    if [ "$arg" = "--clean" ]; then
        echo "Cleaning sanitizer build..."
        make -C src OBJDIR="$SAN_OBJDIR" DMS_BINARY="$SAN_BINARY" clean-server || exit 1
    else
        MAKE_ARGS+=("$arg")
    fi
done

# -fsanitize needs to reach both the compiler and the linker.  These are
# appended to the normal flags rather than replacing them, so the sanitizer
# build keeps the full warning profile.  _FORTIFY_SOURCE is turned off because
# it conflicts with ASan's own interceptors.
make -C src \
    OBJDIR="$SAN_OBJDIR" \
    DMS_BINARY="$SAN_BINARY" \
    EXTRA_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0" \
    EXTRA_LDFLAGS="-fsanitize=address,undefined" \
    "${MAKE_ARGS[@]}" dms_new || exit 1

echo
echo "Sanitizer build written to bin/server/dms_san (runtime dms untouched)."
