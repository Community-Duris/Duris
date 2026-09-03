#!/usr/bin/env python3

import hashlib
import os
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
REPAIR = ROOT / "migrations/repair_missing_combat_baselines.sh"


def environment(tool_dir: pathlib.Path, classification: str) -> dict[str, str]:
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
            "CLASSIFICATION_ROWS": classification,
            "CHECK_ROW": "1\t0\t0\t0",
        }
    )
    return values


with tempfile.TemporaryDirectory(prefix="duris-combat-baseline-") as temporary:
    private = pathlib.Path(temporary)
    private.chmod(0o700)
    mysql = private / "mysql"
    mysql.write_text(
        "#!/usr/bin/env bash\n"
        "if [[ \"$1\" == '--help' ]]; then echo '--ssl-mode'; exit 0; fi\n"
        "query=${!#}\n"
        "if [[ \"$query\" == *'INSERT INTO combat_frag_baseline'* ]]; then exit 0; fi\n"
        "if [[ \"$query\" == *'COALESCE(SUM(wallet.pid IS NULL)'* ]]; then\n"
        "  printf '%s\\n' \"$CHECK_ROW\"; exit 0\n"
        "fi\n"
        "printf '%s\\n' \"$CLASSIFICATION_ROWS\"\n"
    )
    mysql.chmod(0o700)

    safe_artifact = private / "safe.tsv"
    safe_row = "101\t7\t0\t0\t0\tsafe_no_history\t7\t0"
    env = environment(private, safe_row)
    classified = subprocess.run(
        [str(REPAIR), "--classify", str(safe_artifact)],
        cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    assert classified.returncode == 0, classified.stdout
    assert "safe_no_history=1" in classified.stdout
    assert safe_artifact.stat().st_mode & 0o077 == 0
    digest = hashlib.sha256(safe_artifact.read_bytes()).hexdigest()
    env.update(
        {
            "WRITERS_QUIESCED": "TRUE",
            "COMBAT_BASELINE_BACKUP_ID": "test-backup-generation",
        }
    )
    applied = subprocess.run(
        [str(REPAIR), "--apply", str(safe_artifact), digest],
        cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    assert applied.returncode == 0, applied.stdout
    assert "approved_rows=1" in applied.stdout
    assert (private / "safe.tsv.applied").stat().st_mode & 0o077 == 0

    repeated = subprocess.run(
        [str(REPAIR), "--apply", str(safe_artifact), digest],
        cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    assert repeated.returncode == 0, repeated.stdout

    history_artifact = private / "history.tsv"
    history_row = "102\t9\t1\t1\t2\tledger_history_requires_review\t7\t0"
    history_env = environment(private, history_row)
    history_classified = subprocess.run(
        [str(REPAIR), "--classify", str(history_artifact)],
        cwd=ROOT, env=history_env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    assert history_classified.returncode == 0, history_classified.stdout
    history_digest = hashlib.sha256(history_artifact.read_bytes()).hexdigest()
    history_env.update(
        {
            "WRITERS_QUIESCED": "TRUE",
            "COMBAT_BASELINE_BACKUP_ID": "test-backup-generation",
        }
    )
    refused = subprocess.run(
        [str(REPAIR), "--apply", str(history_artifact), history_digest],
        cwd=ROOT, env=history_env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    assert refused.returncode == 1, refused.stdout
    assert "history or ambiguity" in refused.stdout

source = REPAIR.read_text()
assert "CHECK(ok=1)" in source
assert "baseline.opening_frags=approved.opening_frags" in source
assert "refusing combat baseline repair outside" in source
assert "WRITERS_QUIESCED" in source
assert "COMBAT_BASELINE_BACKUP_ID" in source
assert "INSERT IGNORE" not in source

print("combat frag baseline classification and repair workflow passed")
