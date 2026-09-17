"""Create protected native backups for an explicitly selected recovery target.

No restore, service control, or database mutation is performed here. Rollback
requires its own approval and the same stopped-writer maintenance boundary.
"""
from __future__ import annotations

from datetime import datetime, timezone
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
from typing import Any, Callable

from player_death_restitution_target import (
    BACKUP_RECEIPT_FORMAT, TargetError, TargetPolicy, protected_file_digest,
    verify_backup_receipt,
)


def create_backup(
    db: Any, policy: TargetPolicy, receipt_path: Path,
    assert_quiescent: Callable[[], None], *, dump_binary: str | None = None,
) -> dict[str, Any]:
    """Export through the real vendor utility and receipt the exact file/target.

    assert_quiescent must be the recovery CLI's real process/database check,
    not a supplied attestation. The application must bind this returned receipt
    into the approved plan and revalidate its target/hash before mutation.
    """
    target = policy.identity(db)
    boundary = policy.require_maintenance()
    assert_quiescent()
    binary = dump_binary or db.env.get("MYSQLDUMP_BIN") or shutil.which("mysqldump")
    if not binary:
        raise TargetError("native mysqldump client is required for a recovery backup")
    try:
        help_result = subprocess.run([binary, "--help"], capture_output=True, text=True,
                                     timeout=20, check=False, env=db.env)
    except (OSError, subprocess.SubprocessError) as exc:
        raise TargetError("native backup client could not be inspected") from exc
    if help_result.returncode:
        raise TargetError("native backup client could not be inspected")
    flags = ["--single-transaction", "--skip-lock-tables", "--skip-add-locks",
             "--routines", "--events", "--triggers", "--hex-blob",
             "--no-tablespaces", "--comments", "--dump-date", "--complete-insert"]
    if "set-gtid-purged" in help_result.stdout:
        flags.append("--set-gtid-purged=OFF")
    # Match the selected client dialect rather than blindly reuse mysql flags.
    ssl = ["--ssl-mode=PREFERRED"] if "ssl-mode" in help_result.stdout else ["--skip-ssl"]
    command = [binary, *ssl, "--protocol=TCP", "-h", policy.host, "-P", policy.port,
               "-u", db.env["DB_USER"], *flags, policy.database]
    receipt_path = receipt_path.expanduser().absolute()
    parent = receipt_path.parent
    parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    parent_stat = parent.lstat()
    if (not stat.S_ISDIR(parent_stat.st_mode) or parent_stat.st_uid != os.getuid()
            or parent_stat.st_mode & 0o022):
        raise TargetError("backup directory must be owned by the operator and not externally writable")
    dump_path = receipt_path.with_name(receipt_path.name + ".sql")
    if receipt_path.exists() or receipt_path.is_symlink() or dump_path.exists() or dump_path.is_symlink():
        raise TargetError("refusing to replace an existing backup artifact")
    fd, tmp_name = tempfile.mkstemp(prefix=".restitution-backup-", dir=parent)
    temporary = Path(tmp_name)
    manifest_tmp: Path | None = None
    try:
        os.fchmod(fd, 0o600)
        with os.fdopen(fd, "wb") as output:
            try:
                result = subprocess.run(command, stdout=output, stderr=subprocess.PIPE,
                                        env=db.env, timeout=1800, check=False)
            except (OSError, subprocess.SubprocessError) as exc:
                raise TargetError("native recovery backup failed") from exc
            output.flush()
            os.fsync(output.fileno())
        if result.returncode:
            raise TargetError("native recovery backup failed; no completed receipt was issued")
        with temporary.open("rb") as source:
            source.seek(max(0, temporary.stat().st_size - 4096))
            ending = source.read()
        if b"-- Dump completed on " not in ending:
            raise TargetError("native backup has no completion footer")
        policy.require_maintenance()
        assert_quiescent()
        if policy.identity(db) != target:
            raise TargetError("database identity changed during backup")
        sha256 = protected_file_digest(temporary)
        receipt = {
            "format": BACKUP_RECEIPT_FORMAT, "target": target,
            "maintenance_boundary": boundary, "dump_path": str(dump_path),
            "sha256": sha256, "dump_exit_code": 0, "complete": True,
            "created_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        }
        # Hard-link installation is atomic and never overwrites another file.
        os.link(temporary, dump_path, follow_symlinks=False)
        mfd, manifest_name = tempfile.mkstemp(prefix=".restitution-backup-receipt-", dir=parent)
        manifest_tmp = Path(manifest_name)
        with os.fdopen(mfd, "w", encoding="utf-8") as output:
            os.fchmod(output.fileno(), 0o600)
            json.dump(receipt, output, sort_keys=True, indent=2)
            output.write("\n")
            output.flush()
            os.fsync(output.fileno())
        os.link(manifest_tmp, receipt_path, follow_symlinks=False)
        dir_fd = os.open(parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
        try:
            os.fsync(dir_fd)
        finally:
            os.close(dir_fd)
        verify_backup_receipt(receipt, target, boundary)
        return receipt
    except OSError as exc:
        raise TargetError("backup artifact could not be installed securely") from exc
    finally:
        temporary.unlink(missing_ok=True)
        if manifest_tmp is not None:
            manifest_tmp.unlink(missing_ok=True)
