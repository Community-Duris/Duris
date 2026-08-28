#!/usr/bin/env bash
set -euo pipefail

cat >&2 <<'EOF'
ERROR: scripts/delete_corpses.sh is retired and performs no cleanup.

The script targeted an obsolete world-recovery format and could remove unrelated floor
recovery data. Do not restore or reuse its former implementation.

Corpse cleanup requires a replacement that fences the server, identifies exact
authoritative corpse UIDs, preserves unrelated recovery data, creates a verified backup,
and validates both persistent stores after the operation.
EOF

exit 2
