#!/usr/bin/env bash
set -euo pipefail

# Restore one flat-file backup generation into an empty state root and verify it
# against the manifest written by scripts/backup_pfiles.sh. The server replays a
# captured pending authority transaction on its next boot; this script only
# reports that it is present.

usage() {
  echo "usage: $(basename "$0") <backup-generation-dir> <target-state-dir>" >&2
  exit 2
}

if [[ $# -ne 2 ]]; then
  usage
fi

umask 077

BACKUP_DIR="$1"
TARGET_DIR="$2"

if [[ "$BACKUP_DIR" != /* || "$TARGET_DIR" != /* ]]; then
  echo "Both paths must be absolute" >&2
  exit 2
fi
if [[ -L "$BACKUP_DIR" || ! -d "$BACKUP_DIR" ]]; then
  echo "Backup generation must be a real directory" >&2
  exit 1
fi
MANIFEST="$BACKUP_DIR/MANIFEST.sha256"
if [[ ! -f "$MANIFEST" ]]; then
  echo "Backup generation has no MANIFEST.sha256; refusing to restore" >&2
  exit 1
fi
if ! head -n 1 -- "$MANIFEST" | grep -q '^# duris-flatfile-backup-manifest 1$'; then
  echo "Unrecognized backup manifest format" >&2
  exit 1
fi

if [[ -L "$TARGET_DIR" ]]; then
  echo "Target state root must not be a symbolic link" >&2
  exit 1
fi
if [[ -e "$TARGET_DIR" ]]; then
  if [[ ! -d "$TARGET_DIR" ]]; then
    echo "Target state root must be a directory" >&2
    exit 1
  fi
  if [[ -n "$(ls -A -- "$TARGET_DIR")" ]]; then
    echo "Target state root must be empty; restore into a fresh root" >&2
    exit 1
  fi
else
  install -d -m 700 -- "$TARGET_DIR"
fi
chmod 0700 -- "$TARGET_DIR"

verify_tree() {
  local root="$1"
  ( cd "$root" && grep -v '^#' -- "$MANIFEST" | sha256sum --quiet --check - )
}

if ! verify_tree "$BACKUP_DIR"; then
  echo "Backup generation does not match its manifest; refusing to restore" >&2
  exit 1
fi

if ! cp -a -- "$BACKUP_DIR/." "$TARGET_DIR/"; then
  echo "Restore copy failed" >&2
  exit 1
fi
rm -f -- "$TARGET_DIR/MANIFEST.sha256"

if ! verify_tree "$TARGET_DIR"; then
  echo "Restored state does not match the manifest" >&2
  exit 1
fi

find "$TARGET_DIR" -type d -exec chmod 0700 {} +
find "$TARGET_DIR" -type f -exec chmod 0600 {} +
sync -f "$TARGET_DIR"

GENERATION="$(grep -m1 '^# generation:' -- "$MANIFEST" | awk '{print $3}')"
PENDING="$(grep -m1 '^# pending-transaction:' -- "$MANIFEST" | awk '{print $3}')"
echo "Restored flat-file generation ${GENERATION:-unknown} into $TARGET_DIR"
if [[ "$PENDING" == "yes" ]]; then
  echo "Generation carries a pending authority transaction; the server replays it at boot."
fi
