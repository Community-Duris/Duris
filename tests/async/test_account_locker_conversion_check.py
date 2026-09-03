#!/usr/bin/env python3

import os
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
CHECK = ROOT / "migrations/check_flatfile_account_locker_conversion.sh"


def run_check(row: str, expected: int = 114) -> subprocess.CompletedProcess[str]:
    with tempfile.TemporaryDirectory(prefix="duris-locker-conversion-") as temporary:
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
            [str(CHECK), "--expect-count", str(expected)],
            cwd=ROOT,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )


passing = run_check("114\t0\t0\t0\t0\t0\t0")
assert passing.returncode == 0, passing.stdout
assert "account_lockers=114" in passing.stdout
assert "unmatched_typed_owner=0" in passing.stdout

bad_shape = run_check("114\t0\t0\t0\t1\t0\t0")
assert bad_shape.returncode == 1, bad_shape.stdout
assert "invalid_item_shape=1" in bad_shape.stdout

bad_count = run_check("113\t0\t0\t0\t0\t0\t0")
assert bad_count.returncode == 1, bad_count.stdout

malformed = run_check("account.name\t0\t0\t0\t0\t0\t0")
assert malformed.returncode == 2, malformed.stdout
assert "malformed aggregate output" in malformed.stdout

source = CHECK.read_text()
assert "FROM accounts a" in source
assert "owner_map.locker_name" in source
assert "SELECT *" not in source
assert not any(token in source for token in ("INSERT ", "UPDATE ", "DELETE ", "REPLACE "))

print("account locker conversion aggregate check passed")
