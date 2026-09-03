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
import tempfile

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

# Sourcing executes the file, so a .env anyone else can write is refused. The probe
# still runs against the documented default: this script is the container HEALTHCHECK,
# and exiting over a permission bit would cycle a healthy container.
assert "env_health_url_safe" in text, (
    "healthcheck.sh must validate .env metadata before sourcing it"
)

with tempfile.TemporaryDirectory() as temporary:
    fake_root = pathlib.Path(temporary)
    (fake_root / "scripts").mkdir()
    fake_script = fake_root / "scripts/healthcheck.sh"
    fake_script.write_bytes(SCRIPT.read_bytes())
    fake_script.chmod(0o755)
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        env_port = probe.getsockname()[1]
    env_file = fake_root / ".env"
    env_file.write_text(f"DURIS_HEALTH_URL=http://127.0.0.1:{env_port}/health\n")
    probe_env = {"PATH": "/usr/bin:/bin"}

    env_file.chmod(0o600)
    safe = subprocess.run([str(fake_script)], capture_output=True, text=True, env=probe_env)
    assert str(env_port) in safe.stderr, (
        "healthcheck.sh ignored a correctly owned 0600 .env:\n" + safe.stderr
    )

    env_file.chmod(0o644)
    unsafe = subprocess.run([str(fake_script)], capture_output=True, text=True, env=probe_env)
    assert str(env_port) not in unsafe.stderr, (
        "healthcheck.sh sourced a group/world-readable .env:\n" + unsafe.stderr
    )
    assert "chmod 600" in unsafe.stderr, (
        "healthcheck.sh must say why it ignored .env:\n" + unsafe.stderr
    )
    assert "4050" in unsafe.stderr, (
        "healthcheck.sh must fall back to the documented default:\n" + unsafe.stderr
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
