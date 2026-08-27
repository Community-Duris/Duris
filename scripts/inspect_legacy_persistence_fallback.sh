#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 [--quarantine] <legacy-fallback-file>" >&2
    exit 2
}

quarantine=0
if [[ "${1:-}" == "--quarantine" ]]; then
    quarantine=1
    shift
fi
[[ $# == 1 ]] || usage
target="$1"
[[ -f "$target" && ! -L "$target" ]] || { echo "refusing non-regular or symlink fallback target" >&2; exit 2; }

digest=$(sha256sum -- "$target" | awk '{print $1}')
read -r item scalar large other < <(awk '
BEGIN { item=scalar=large=other=0 }
/^PERSISTENCE_ITEM_EVENT\|/ { item++; next }
/^PERSISTENCE_SCALAR_EVENT\|/ { scalar++; next }
/^PERSISTENCE_LARGE_EVENT\|/ { large++; next }
{ other++ }
END { print item, scalar, large, other }
' "$target")
size=$(stat -c '%s' -- "$target")
printf 'legacy_fallback sha256=%s bytes=%s item=%s scalar=%s large=%s other=%s\n' \
    "$digest" "$size" "$item" "$scalar" "$large" "$other"

if (( quarantine )); then
    timestamp=$(date -u +%Y%m%dT%H%M%SZ)
    destination="${target}.quarantine.${timestamp}.${RANDOM}"
    mv -- "$target" "$destination"
    chmod 0600 "$destination"
    printf 'quarantined=%s\n' "$destination"
fi
