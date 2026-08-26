#!/usr/bin/env bash
#
# Point this clone's git hooks at the versioned hooks in scripts/git-hooks.
#
#   ./scripts/install-hooks.sh              # install
#   ./scripts/install-hooks.sh --uninstall  # revert to .git/hooks
set -euo pipefail

# Always run from the repository root so relative paths resolve correctly.
cd "$(dirname "$0")/.." || exit 1

if [[ "${1:-}" == "--uninstall" ]]; then
  git config --unset core.hooksPath || true
  echo "Hooks uninstalled; git is back to .git/hooks."
  exit 0
fi

if [[ -n "${1:-}" ]]; then
  echo "Usage: scripts/install-hooks.sh [--uninstall]" >&2
  exit 2
fi

# core.hooksPath replaces .git/hooks wholesale, so warn if anything lives there.
existing="$(find .git/hooks -maxdepth 1 -type f ! -name '*.sample' 2>/dev/null | wc -l)"
if (( existing > 0 )); then
  echo "WARNING: .git/hooks already contains $existing hook(s); they will be bypassed." >&2
  find .git/hooks -maxdepth 1 -type f ! -name '*.sample' -printf '  %f\n' >&2
fi

git config core.hooksPath scripts/git-hooks
echo "Installed: core.hooksPath = scripts/git-hooks"
echo "  pre-commit: auto-formats staged C/C++ changes"
echo "  bypass one commit with: git commit --no-verify"
