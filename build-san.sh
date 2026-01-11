#!/bin/bash
# build with address sanitizer and undefined behavior sanitizer

# Handle --clean flag
if [[ "$*" == *"--clean"* ]]; then
    echo "Cleaning build..."
    cd src/
    rm -f *.o dms_new
    make clean
    # Remove --clean from arguments passed to make
    MAKE_ARGS=("${@/--clean/}")
else
    MAKE_ARGS=("$@")
fi

cd src/

export CFLAGS="-fsanitize=address,undefined -g -O1"
export LDFLAGS="-fsanitize=address,undefined"

make "${MAKE_ARGS[@]}"

cp dms_new ../dms
