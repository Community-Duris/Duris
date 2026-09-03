"""The health probe must target the configured listener.

The runbook, incident response, and deployment guides all invoke
`scripts/healthcheck.sh` with no environment prepared.  `.env` may move the
health listener off the 4050 default, and cycle_mud.sh reads it from there, so
the probe has to consult the same file or it reports a healthy server as down
at exactly the moment someone is checking whether the server is up.
"""

from __future__ import annotations

import pathlib
import re
import socket
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/healthcheck.sh"

text = SCRIPT.read_text(encoding="utf-8")

# An explicit value from the caller still wins over the file.
assert 'if [[ -z "${DURIS_HEALTH_URL:-}"' in text, (
    "healthcheck.sh must only fall back to .env when DURIS_HEALTH_URL is unset"
)
assert '"$root/.env"' in text, "healthcheck.sh must read DURIS_HEALTH_URL from .env"
assert 'health_url="${DURIS_HEALTH_URL:-http://127.0.0.1:4050/health}"' in text, (
    "healthcheck.sh must keep its documented default"
)
# Sourcing happens in a subshell so the rest of .env, including credentials,
# never enters this process environment.
subshell = re.search(r'DURIS_HEALTH_URL="\$\(\n(.*?)\n\t\)"', text, re.S)
assert subshell and "source" in subshell.group(1), (
    "healthcheck.sh must source .env inside a command substitution subshell"
)

# A caller-supplied URL is probed verbatim rather than silently replaced.
with socket.socket() as probe:
    probe.bind(("127.0.0.1", 0))
    closed_port = probe.getsockname()[1]

result = subprocess.run(
    [str(SCRIPT)],
    cwd=ROOT,
    env={
        "PATH": "/usr/bin:/bin",
        "DURIS_HEALTH_URL": f"http://127.0.0.1:{closed_port}/health",
    },
    capture_output=True,
    text=True,
)
assert result.returncode != 0, "healthcheck.sh reported success against a closed port"
assert str(closed_port) in result.stderr, (
    "healthcheck.sh did not probe the caller-supplied URL:\n" + result.stderr
)
