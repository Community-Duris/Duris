#!/usr/bin/env python3
import gzip
import os
from pathlib import Path
import stat
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "backup_pfiles.sh"
CYCLE = (ROOT / "scripts" / "cycle_mud.sh").read_text(encoding="ascii")


def write_executable(path: Path, text: str) -> None:
    path.write_text(text, encoding="ascii")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def run_backup(base: Path, mode: str, fail_gzip: bool = False) -> subprocess.CompletedProcess[str]:
    base.mkdir()
    env_file = base / "backup.env"
    env_file.write_text(
        "ENVIRONMENT=local\n"
        "DB_HOST=127.0.0.1\n"
        "DB_PORT=3306\n"
        "DB_USER=tester\n"
        "DB_PASSWD=secret\n"
        "DB_NAME=duris_test\n"
        "DB_ALLOWED_TARGETS=127.0.0.1/duris_test\n"
        "DB_TLS=FALSE\n"
        "DB_SSL_CA=\n"
        "DB_SOCKET=\n"
        "REDIS=TRUE\n",
        encoding="ascii",
    )
    env_file.chmod(0o600)

    stubs = base / "stubs"
    stubs.mkdir()
    write_executable(
        stubs / "mysqldump",
        "#!/usr/bin/env bash\n"
        "case \"${DUMP_MODE:?}\" in\n"
        "  success)\n"
        "    printf '%s\\n' 'CREATE TABLE `accounts` (' 'CREATE TABLE `player_data` (' 'CREATE TABLE `ships` ('\n"
        "    ;;\n"
        "  invalid) printf '%s\\n' '-- incomplete dump' ;;\n"
        "  failure) exit 23 ;;\n"
        "esac\n",
    )
    if fail_gzip:
        write_executable(stubs / "gzip", "#!/usr/bin/env bash\nexit 24\n")

    backup_dir = base / "backups"
    env = os.environ.copy()
    env.update(
        {
            "BACKUP_ENV_FILE": str(env_file),
            "DATABASE_BACKUP_DIR": str(backup_dir),
            "DUMP_MODE": mode,
            "PATH": f"{stubs}:/usr/bin:/bin",
        }
    )
    return subprocess.run(
        [str(SCRIPT)],
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


with tempfile.TemporaryDirectory(prefix="duris-backup-test-") as temp:
    base = Path(temp)

    success = run_backup(base / "success", "success")
    assert success.returncode == 0, success.stdout
    published = list((base / "success" / "backups").glob("*.sql.gz"))
    assert len(published) == 1, success.stdout
    with gzip.open(published[0], "rt", encoding="ascii") as stream:
        dump = stream.read()
    assert "CREATE TABLE `accounts`" in dump
    assert "CREATE TABLE `player_data`" in dump
    assert "CREATE TABLE `ships`" in dump
    assert not list((base / "success" / "backups").glob("*.tmp.*"))

    for case, mode, fail_gzip in (
        ("dump-failure", "failure", False),
        ("invalid-dump", "invalid", False),
        ("gzip-failure", "success", True),
    ):
        result = run_backup(base / case, mode, fail_gzip)
        assert result.returncode != 0, result.stdout
        backup_dir = base / case / "backups"
        assert not list(backup_dir.glob("*.sql.gz")), result.stdout
        assert not list(backup_dir.glob("*.tmp.*")), result.stdout

assert "if ! ./scripts/backup_pfiles.sh; then" in CYCLE
assert 'echo "Required $PERSISTENCE_MODE backup failed; refusing to boot"' in CYCLE
print("backend-selected backup atomicity and failure-propagation checks passed")
