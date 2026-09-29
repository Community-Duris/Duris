#!/usr/bin/env python3
"""Create the explicitly approved local Docker backup policy once."""
from __future__ import annotations

import json
import os
from pathlib import Path
import re
import sys
import tempfile

from persistence_backup import BackupError, policy_load, secure_path


APPROVAL_TOKEN = "local-volume"
CUSTODIAN_PATTERN = re.compile(r"[A-Za-z0-9._@-]{1,128}\Z")
POLICY_ROOT = "/var/lib/duris/db-backups"


class PolicyProvisionError(Exception):
    """A fixed, non-sensitive Docker policy setup error."""


def provision_policy(path: Path, approval: str, custodian: str) -> bool:
    """Create the local policy without replacing an existing operator policy."""
    path = Path(path)
    if not path.is_absolute() or ".." in path.parts:
        raise PolicyProvisionError("BACKUP_POLICY_FILE must be an absolute safe path")

    if path.exists() or path.is_symlink():
        try:
            policy_load(path)
        except (BackupError, OSError, ValueError, TypeError):
            raise PolicyProvisionError(
                "existing Docker backup policy is invalid; refusing to replace it"
            ) from None
        return False

    if approval != APPROVAL_TOKEN:
        raise PolicyProvisionError("explicit local Docker backup approval is required")
    if not CUSTODIAN_PATTERN.fullmatch(custodian):
        raise PolicyProvisionError("a valid Docker backup custodian is required")

    secure_path(path.parent, True)
    template = Path(__file__).with_name("backup_policy.example.json")
    policy = json.loads(template.read_text(encoding="utf-8"))
    policy["approved"] = True
    policy["custodian"] = custodian
    policy["root"] = POLICY_ROOT

    descriptor, temporary_name = tempfile.mkstemp(
        prefix=".docker-backup-policy-", dir=path.parent
    )
    temporary_path = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as stream:
            json.dump(policy, stream, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        try:
            os.link(temporary_path, path, follow_symlinks=False)
        except FileExistsError:
            try:
                policy_load(path)
            except (BackupError, OSError, ValueError, TypeError):
                raise PolicyProvisionError(
                    "existing Docker backup policy is invalid; refusing to replace it"
                ) from None
            return False
        directory_fd = os.open(
            path.parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
        )
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
    finally:
        temporary_path.unlink(missing_ok=True)

    policy_load(path)
    return True


def main() -> int:
    policy_file = os.environ.get("BACKUP_POLICY_FILE", "")
    approval = os.environ.get("DURIS_DOCKER_BACKUP_APPROVAL", "")
    custodian = os.environ.get("DURIS_DOCKER_BACKUP_CUSTODIAN", "")
    if not policy_file:
        print(
            "Docker backup policy setup failed: BACKUP_POLICY_FILE is required",
            file=sys.stderr,
        )
        return 1
    try:
        created = provision_policy(Path(policy_file), approval, custodian)
    except (PolicyProvisionError, BackupError, OSError, ValueError, TypeError) as error:
        if isinstance(error, (BackupError, PolicyProvisionError)):
            detail = str(error)
        else:
            detail = "operation_failed"
        print(f"Docker backup policy setup failed: {detail}", file=sys.stderr)
        return 1
    message = "Created" if created else "Validated existing"
    print(f"{message} local Docker backup policy: {policy_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
