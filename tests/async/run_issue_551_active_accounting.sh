#!/usr/bin/env bash
# Build the test-only admission/fault projection around the real server.
# Use only isolated state and explicitly opted-in disposable loopback SQL.
set -euo pipefail
cd "$(dirname "$0")/../.."
mode=${1:-file}
case "$mode" in
    file) backend=flatfile; includes=(-D__NO_MYSQL__ -Isrc/no_mysql) ;;
    sql)
        [[ ${TEST_DB_DISPOSABLE:-} == 1 && ${TEST_DB_HOST:-} =~ ^(127\.0\.0\.1|localhost)$ ]] || {
            echo 'SQL gameplay qualification requires disposable loopback database opt-in' >&2; exit 1;
        }
        backend=mariadb; includes=(-I/usr/include/mysql) ;;
    *) echo 'usage: run_issue_551_active_accounting.sh file|sql' >&2; exit 1 ;;
esac
mkdir -p bin/tests
object="$PWD/bin/tests/issue551-active-$backend.o"
binary="$PWD/bin/server/dms_551_active_$backend"
g++ -std=c++20 -Isrc "${includes[@]}" -c tests/async/issue551_active_projection_fixture.cpp -o "$object"
flags="$object -Wl,--wrap=_ZN27economic_gameplay_authority6activeEv -Wl,--wrap=_Z52critical_command_coordinator_acknowledge_publicationRK21critical_operation_id -Wl,--wrap=_Z6numberii"
if [[ $backend == mariadb ]]; then flags+=' -Wl,--wrap=_Z26sql_economic_runtime_startv'; fi
# EXTRA_LDFLAGS is not a make prerequisite. Always relink the selected qualifier
# when the fault fixture changes, without forcing a clean server rebuild.
make -C src -j"${BUILD_JOBS:-4}" PERSISTENCE_BACKEND="$backend" DMS_BINARY="$PWD/bin/server/dms_551_production_$backend"
touch "bin/objects/server/$backend/development/economy/economic_gameplay_authority.o"
make -C src -j"${BUILD_JOBS:-4}" PERSISTENCE_BACKEND="$backend" DMS_BINARY="$binary" EXTRA_LDFLAGS="$flags"
if [[ $mode == file ]]; then
    python3 -c 'import sys; sys.path.insert(0,"tests/async"); import test_flatfile_combat_journey as journey; journey.build_inspector()'
    ISSUE551_RETAIN_FIXTURE_BINARY=1 python3 tests/async/test_issue_551_flatfile_craft.py
fi
python3 tests/async/run_alchemist_crafting_journey.py "$binary" "$mode" --active-alchemy-only
