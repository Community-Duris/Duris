#!/usr/bin/env python3

"""Point-in-time flat-file backup drill: a backup captures one generation with a
verifiable manifest, refuses to publish a mixed-generation copy, and restores
into an empty root byte-for-byte."""

import hashlib
import os
import pathlib
import subprocess
import tempfile
import time


ROOT = pathlib.Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def provision(state_root: pathlib.Path) -> None:
    for directory in (
        "metadata",
        "identities",
        "identities/accounts",
        "identities/names",
        "players",
        "operations",
        "operations/wal",
        "domains",
        "manifests",
    ):
        (state_root / directory).mkdir(parents=True, exist_ok=True, mode=0o700)
    (state_root / "identities/names/catalog.identity").write_bytes(b"catalog-generation-1")
    (state_root / "identities/accounts/6163636f756e74.acct").write_bytes(b"account-generation-1")
    (state_root / "players/1.player").write_bytes(b"player-generation-1")
    (state_root / "domains/boons.domain").write_bytes(b"boons-generation-1")
    for path in state_root.rglob("*"):
        path.chmod(0o700 if path.is_dir() else 0o600)
    state_root.chmod(0o700)


def run_backup(state_root: pathlib.Path, backup_root: pathlib.Path, env_extra=None):
    environment = dict(os.environ)
    environment.update(
        {
            "PERSISTENCE_MODE": "flatfile-primary",
            "FLATFILE_STATE_DIR": str(state_root),
            "FLATFILE_BACKUP_DIR": str(backup_root),
            "BACKUP_ENV_FILE": str(state_root / "absent.env"),
        }
    )
    environment.update(env_extra or {})
    return subprocess.run(
        [str(ROOT / "scripts/backup_pfiles.sh")],
        cwd=ROOT,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=300,
    )


EXCLUDED = {
    "MANIFEST.sha256",
    ".identity.lock",
    ".accounts.lock",
    ".critical-authority.lock",
}


def digest_tree(root: pathlib.Path) -> dict:
    digests = {}
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.name not in EXCLUDED:
            digests[str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return digests


with tempfile.TemporaryDirectory(prefix="duris-backup-drill-") as temporary:
    temporary_path = pathlib.Path(temporary)
    state_root = temporary_path / "state"
    backup_root = temporary_path / "backups"
    state_root.mkdir(mode=0o700)
    provision(state_root)

    result = run_backup(state_root, backup_root)
    require(result.returncode == 0, "flat-file backup failed:\n" + result.stdout)
    generations = sorted(path for path in backup_root.iterdir() if path.is_dir())
    require(len(generations) == 1, f"expected one captured generation: {generations}")
    generation = generations[0]

    manifest = generation / "MANIFEST.sha256"
    require(manifest.is_file(), "backup published without a generation manifest")
    manifest_text = manifest.read_text()
    require(
        manifest_text.splitlines()[0] == "# duris-flatfile-backup-manifest 1",
        "manifest is missing its format header:\n" + manifest_text,
    )
    require("# pending-transaction: no" in manifest_text, "manifest omitted transaction state")
    listed = {
        line.split(maxsplit=1)[1].removeprefix("./")
        for line in manifest_text.splitlines()
        if line and not line.startswith("#")
    }
    require(
        listed == set(digest_tree(state_root)),
        f"manifest does not cover the captured generation: {listed}",
    )
    require(
        digest_tree(generation) == digest_tree(state_root),
        "captured generation does not match the source tree",
    )

    # A writer holding an authority lock must block the backup rather than let it
    # copy a mixed generation.
    authority_lock_path = state_root / "domains/.critical-authority.lock"
    authority_lock_path.touch(mode=0o600)
    time.sleep(1.1)
    contended = subprocess.Popen(["flock", "-x", str(authority_lock_path), "-c", "sleep 10"])
    try:
        result = run_backup(state_root, backup_root, {"FLATFILE_LOCK_WAIT": "1"})
        require(
            result.returncode != 0 and "quiesce" in result.stdout,
            "backup published a generation while a writer held the authority lock:\n"
            + result.stdout,
        )
        require(
            sorted(path for path in backup_root.iterdir() if path.is_dir()) == [generation],
            "a failed backup left a partial generation behind",
        )
    finally:
        contended.wait(timeout=30)

    restore_root = temporary_path / "restored"
    restore = subprocess.run(
        [str(ROOT / "scripts/restore_flatfile_backup.sh"), str(generation), str(restore_root)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=300,
    )
    require(restore.returncode == 0, "restore drill failed:\n" + restore.stdout)
    require(
        digest_tree(restore_root) == digest_tree(state_root),
        "restored root does not match the captured generation",
    )
    require(
        not (restore_root / "MANIFEST.sha256").exists(),
        "restore left the manifest inside the state root",
    )

    tampered = temporary_path / "tampered"
    subprocess.run(["cp", "-a", str(generation), str(tampered)], check=True)
    (tampered / "players/1.player").write_bytes(b"player-generation-tampered")
    rejected = subprocess.run(
        [str(ROOT / "scripts/restore_flatfile_backup.sh"), str(tampered), str(temporary_path / "rejected")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=300,
    )
    require(
        rejected.returncode != 0 and "manifest" in rejected.stdout,
        "restore accepted a generation that does not match its manifest:\n" + rejected.stdout,
    )

print("flat-file point-in-time backup manifest and restore drill passed")
