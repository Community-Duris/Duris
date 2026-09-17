#!/usr/bin/env bash
# Run inside the task's disposable SQL-enabled build container.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$ROOT"
OUT=${1:?usage: build_player_death_restitution_runtime.sh OUTPUT_PATH}
make -C src OBJDIR=/tmp/restitution-project-objects \
    /tmp/restitution-project-objects/player/player_load_repository.o \
    /tmp/restitution-project-objects/player/player_snapshot_repository.o \
    /tmp/restitution-project-objects/sql/sql.o \
    /tmp/restitution-project-objects/sql/sql_pool.o
# Link the real repositories and materializer. The checker supplies only the
# small game-world hooks described in its header, not substitute SQL loaders.
g++ -std=c++20 -O1 -Wall -Wextra -Wpedantic -Werror \
    -ffunction-sections -fdata-sections -Isrc -I/usr/include/mysql -I/usr/include/libxml2 \
    tests/async/player_death_restitution_runtime_check.cpp \
    src/player/player_load_repository.c src/player/player_load_topology.c \
    src/player/player_load_items.c src/player/player_snapshot_codec.c \
    src/player/player_snapshot_repository.c src/persistence/persistence_observability.c \
    src/sql/item_extra_descr_codec.c src/sql/sql_player.c src/sql/sql_pool.c \
    -Wl,--gc-sections -lmysqlclient -lcrypto -lpthread -o "$OUT"
g++ -std=c++20 -O1 -Wall -Wextra -Wpedantic -Werror \
    -ffunction-sections -fdata-sections -Isrc -I/usr/include/mysql -I/usr/include/libxml2 \
    tests/async/player_death_restitution_guard_native.cpp \
    src/sql/sql_pool.c src/persistence/persistence_observability.c \
    -Wl,--gc-sections -lmysqlclient -lcrypto -lpthread -o "${OUT}-guard"
