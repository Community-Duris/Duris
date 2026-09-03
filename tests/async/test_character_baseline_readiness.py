#!/usr/bin/env python3

import os
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
CHECK = ROOT / "migrations/check_character_baseline_readiness.sh"


def run_check(row: str) -> subprocess.CompletedProcess[str]:
    with tempfile.TemporaryDirectory(prefix="duris-baseline-readiness-") as temporary:
        tool_dir = pathlib.Path(temporary)
        mysql = tool_dir / "mysql"
        mysql.write_text(
            "#!/usr/bin/env bash\n"
            "if [[ \"$1\" == '--help' ]]; then echo '--ssl-mode'; exit 0; fi\n"
            "printf '%s\\n' \"$CHECK_ROW\"\n"
        )
        mysql.chmod(0o700)
        environment = os.environ.copy()
        environment.update(
            {
                "PATH": f"{tool_dir}:{environment['PATH']}",
                "DB_HOST": "127.0.0.1",
                "DB_PORT": "3306",
                "DB_USER": "test",
                "DB_PASSWD": "test",
                "DB_NAME": "duris_test",
                "CHECK_ROW": row,
            }
        )
        return subprocess.run(
            [str(CHECK)], cwd=ROOT, env=environment, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )


ready = run_check("20\t0\t0\t0")
assert ready.returncode == 0, ready.stdout
assert "eligible_characters=20" in ready.stdout

missing_combat = run_check("20\t0\t0\t2")
assert missing_combat.returncode == 1, missing_combat.stdout
assert "combat_frag_baseline_missing=2" in missing_combat.stdout

missing_wallet = run_check("20\t1\t0\t0")
assert missing_wallet.returncode == 1, missing_wallet.stdout

malformed = run_check("character\t0\t0\t0")
assert malformed.returncode == 2, malformed.stdout

source = CHECK.read_text()
assert "player.active=1" in source
assert "mapping.deleted_at IS NULL" in source
assert "mapping.blocked=0" in source
assert "SELECT DISTINCT player.pid" in source

print("active character baseline readiness check passed")
