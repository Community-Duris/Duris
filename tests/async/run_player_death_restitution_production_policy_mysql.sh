#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
IMAGE=${DURIS_PRODUCTION_POLICY_DB_IMAGE:-mariadb:10.11}
DURIS_PRODUCTION_POLICY_DB_IMAGE="$IMAGE" PYTHONDONTWRITEBYTECODE=1 \
    python3 "$ROOT/tests/async/test_player_death_restitution_production_policy.py" -v
