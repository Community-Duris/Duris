#!/usr/bin/env python3

import os
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
CHECK = ROOT / "migrations/check_flatfile_account_locker_conversion.sh"


def run_check(
    row: str,
    expected: int = 114,
    *,
    host: str = "127.0.0.1",
    tls: bool = False,
    ca_available: bool = False,
    help_text: str = "--ssl-mode",
    expected_transport: str = "",
) -> subprocess.CompletedProcess[str]:
    """Run the aggregate checker with a validating fake MySQL client."""
    with tempfile.TemporaryDirectory(prefix="duris-locker-conversion-") as temporary:
        tool_dir = pathlib.Path(temporary)
        mysql = tool_dir / "mysql"
        mysql.write_text(
            "#!/usr/bin/env bash\n"
            "if [[ \"${1:-}\" == '--help' ]]; then printf '%s\\n' \"$MYSQL_HELP\"; exit 0; fi\n"
            "if [[ \"${MYSQL_EXPECT_TRANSPORT:-}\" == modern ]]; then\n"
            "  [[ \" $* \" == *' --ssl-mode=VERIFY_IDENTITY '* ]] || exit 90\n"
            "  [[ \" $* \" == *\" --ssl-ca=$DB_SSL_CA \"* ]] || exit 91\n"
            "elif [[ \"${MYSQL_EXPECT_TRANSPORT:-}\" == legacy ]]; then\n"
            "  [[ \" $* \" == *' --ssl-verify-server-cert '* ]] || exit 92\n"
            "  [[ \" $* \" == *\" --ssl-ca=$DB_SSL_CA \"* ]] || exit 93\n"
            "fi\n"
            "printf '%s\\n' \"$CHECK_ROW\"\n"
        )
        mysql.chmod(0o700)
        ca_path = tool_dir / "database-ca.pem"
        if ca_available:
            ca_path.write_text("test CA placeholder\n")
        environment = os.environ.copy()
        environment.update(
            {
                "PATH": f"{tool_dir}:{environment['PATH']}",
                "DB_HOST": host,
                "DB_PORT": "3306",
                "DB_USER": "test",
                "DB_PASSWD": "test",
                "DB_NAME": "duris_test",
                "CHECK_ROW": row,
                "MYSQL_HELP": help_text,
                "MYSQL_EXPECT_TRANSPORT": expected_transport,
            }
        )
        environment.pop("DB_TLS", None)
        environment.pop("DB_SSL_CA", None)
        if tls:
            environment["DB_TLS"] = "TRUE"
        if ca_available:
            environment["DB_SSL_CA"] = str(ca_path)
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

remote_without_ca = run_check("114\t0\t0\t0\t0\t0\t0", host="db.example.test", tls=True)
assert remote_without_ca.returncode == 2, remote_without_ca.stdout
assert "requires TLS and a CA file" in remote_without_ca.stdout

remote_without_verification = run_check(
    "114\t0\t0\t0\t0\t0\t0",
    host="db.example.test",
    tls=True,
    ca_available=True,
    help_text="no verified TLS options",
)
assert remote_without_verification.returncode == 2, remote_without_verification.stdout
assert "cannot verify the remote server identity" in remote_without_verification.stdout

modern_remote = run_check(
    "114\t0\t0\t0\t0\t0\t0",
    host="db.example.test",
    tls=True,
    ca_available=True,
    expected_transport="modern",
)
assert modern_remote.returncode == 0, modern_remote.stdout

legacy_remote = run_check(
    "114\t0\t0\t0\t0\t0\t0",
    host="db.example.test",
    tls=True,
    ca_available=True,
    help_text="--ssl-verify-server-cert",
    expected_transport="legacy",
)
assert legacy_remote.returncode == 0, legacy_remote.stdout

source = CHECK.read_text()
assert "FROM accounts a" in source
assert "owner_map.locker_name" in source
assert "duplicate.state=1" in source
assert "--ssl-mode=VERIFY_IDENTITY" in source
assert "--ssl-verify-server-cert" in source
assert "--ssl-mode=PREFERRED" not in source
assert "--skip-ssl" not in source
assert "SELECT *" not in source
assert not any(token in source for token in ("INSERT ", "UPDATE ", "DELETE ", "REPLACE "))

print("account locker conversion aggregate check passed")
