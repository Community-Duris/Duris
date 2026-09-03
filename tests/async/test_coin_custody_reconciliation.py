#!/usr/bin/env python3
"""Protected reconciliation restores the payload to its authoritative coin UID."""

import hashlib
import os
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
RECONCILE = ROOT / "migrations/reconcile_coin_custody_pair.sh"


def environment(tool_dir: pathlib.Path, row: str) -> dict[str, str]:
    values = os.environ.copy()
    values.update(
        {
            "PATH": f"{tool_dir}:{values['PATH']}",
            "DB_HOST": "127.0.0.1",
            "DB_PORT": "3306",
            "DB_USER": "test",
            "DB_PASSWD": "test",
            "DB_NAME": "duris_test",
            "ENVIRONMENT": "test",
            "PAIR_ROW": row,
        }
    )
    return values


with tempfile.TemporaryDirectory(prefix="duris-coin-custody-") as temporary:
    private = pathlib.Path(temporary)
    private.chmod(0o700)
    mysql = private / "mysql"
    mysql.write_text(
        "#!/usr/bin/env bash\n"
        "if [[ \"$1\" == '--help' ]]; then echo '--ssl-mode'; exit 0; fi\n"
        "query=${!#}\n"
        "if [[ \"$query\" == *'UPDATE player_items SET obj_uid'* ]]; then exit 0; fi\n"
        "printf '%s\\n' \"$PAIR_ROW\"\n"
    )
    mysql.chmod(0o700)

    artifact = private / "pair.tsv"
    row = "41\t700\t9002\t9001\t3\t699\t8000\t8000\t8000\t4\t0\t5\t6\t7\t8"
    env = environment(private, row)
    classified = subprocess.run(
        [str(RECONCILE), "--classify", str(artifact)], cwd=ROOT, env=env,
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    assert classified.returncode == 0, classified.stdout
    assert "candidate_pairs=1" in classified.stdout
    assert artifact.stat().st_mode & 0o077 == 0

    digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
    env.update(
        {
            "WRITERS_QUIESCED": "TRUE",
            "COIN_CUSTODY_BACKUP_ID": "test-backup-generation",
        }
    )
    applied = subprocess.run(
        [str(RECONCILE), "--apply", str(artifact), digest], cwd=ROOT, env=env,
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    assert applied.returncode == 0, applied.stdout
    assert "reconciled_pairs=1" in applied.stdout
    assert (private / "pair.tsv.applied").stat().st_mode & 0o077 == 0

    wrong_digest = subprocess.run(
        [str(RECONCILE), "--apply", str(artifact), "0" * 64], cwd=ROOT, env=env,
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    assert wrong_digest.returncode == 1, wrong_digest.stdout
    assert "checksum" in wrong_digest.stdout

source = RECONCILE.read_text()
assert "UPDATE player_items SET obj_uid=$old_uid" in source
assert "SET value" not in source
assert "DELETE FROM" not in source
assert "item_ownership_baseline" in source
for verb in ("INSERT INTO", "UPDATE", "DELETE FROM"):
    assert f"{verb} item_ownership_ledger" not in source
assert "CHECK(ok=1)" in source
assert "exactly one reviewed owner/payload pair" in source
assert "refusing coin custody reconciliation outside" in source
assert "--ssl-mode=VERIFY_IDENTITY" in source
assert "--skip-ssl" not in source

print("physical coin custody reconciliation is exact and backup-gated")
