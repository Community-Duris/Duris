#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
BUILD=${TMPDIR:-/tmp}/duris-death-restitution-native
mkdir -p "$BUILD"
CXX=${CXX:-g++}
MYSQL_FLAGS=""
if command -v mysql_config >/dev/null 2>&1; then
    MYSQL_FLAGS=$(mysql_config --cflags --libs)
elif command -v mariadb_config >/dev/null 2>&1; then
    MYSQL_FLAGS=$(mariadb_config --cflags --libs)
elif command -v pkg-config >/dev/null 2>&1; then
    MYSQL_FLAGS=$(pkg-config --cflags --libs mysqlclient)
fi
COMMON="-std=c++20 -Wall -Wextra -Werror -I$ROOT/src -I$ROOT"

$CXX $COMMON \
    "$ROOT/src/persistence/player_death_restitution_command.c" \
    "$ROOT/src/persistence/critical_command.c" \
    "$ROOT/tests/async/player_death_restitution_command_unit.cpp" \
    -lcrypto -o "$BUILD/command"
"$BUILD/command"

$CXX $COMMON \
    "$ROOT/src/persistence/player_death_restitution_command.c" \
    "$ROOT/src/persistence/critical_command.c" \
    "$ROOT/src/persistence/player_death_restitution_repository.c" \
    "$ROOT/tests/async/player_death_restitution_repository_unit.cpp" \
    $MYSQL_FLAGS -lcrypto -o "$BUILD/repository"
"$BUILD/repository"

$CXX $COMMON \
    "$ROOT/src/persistence/player_death_restitution_command.c" \
    "$ROOT/src/persistence/critical_command.c" \
    "$ROOT/src/player/player_death_restitution_runtime.c" \
    "$ROOT/tests/async/player_death_restitution_runtime_unit.cpp" \
    -lcrypto -pthread -o "$BUILD/runtime"
"$BUILD/runtime"

$CXX $COMMON \
    "$ROOT/src/persistence/player_death_restitution_command.c" \
    "$ROOT/src/persistence/critical_command.c" \
    "$ROOT/src/player/player_death_restitution_runtime.c" \
    "$ROOT/src/player/player_death_restitution_adapter.c" \
    "$ROOT/tests/async/player_death_restitution_adapter_unit.cpp" \
    -lcrypto -pthread -o "$BUILD/adapter"
"$BUILD/adapter"

# Compile the real game-thread adapter and every touched admission boundary
# with the production Makefile flags.  This is a compile gate only: no server,
# database, or external service is started.
OBJDIR="$BUILD/objects"
make -C "$ROOT/src" -B OBJDIR="$OBJDIR" \
    "$OBJDIR/player/player_save_worker.o" \
    "$OBJDIR/player/player_save_pipeline.o" \
    "$OBJDIR/player/player_load_pipeline.o" \
    "$OBJDIR/player/player_death_restitution_runtime.o" \
    "$OBJDIR/player/player_death_restitution_adapter.o" \
    "$OBJDIR/account/account.o" \
    "$OBJDIR/account/nanny.o" \
    "$OBJDIR/cmd/actinf.o" \
    "$OBJDIR/cmd/actoth.o" \
    "$OBJDIR/core/files.o" \
    "$OBJDIR/net/comm.o"

echo "native death restitution scoped tests passed"
