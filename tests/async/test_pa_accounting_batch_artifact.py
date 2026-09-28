#!/usr/bin/env python3
"""Client-free batch provenance checks; no binary, compiler or DB is executed."""
from __future__ import annotations

import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import pa_accounting_batch_artifact as artifact
import pa_web_recovery_fixture as web_fixture


WEB_SOURCES = (
    "src/net/ws_handlers.c",
    "src/net/websocket.c",
    "src/sql/sql_player.c",
    "src/account/account.c",
    "src/core/files.c",
    "src/player/player_death_conflict_repository.c",
)


class BatchArtifactContracts(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="pa-batch-artifact-contract-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.root = self.directory / "checkout"
        self.root.mkdir()
        self.head = "a" * 40
        self.binary = self.directory / "binary-fixture"
        self.binary.write_bytes(b"Client-free provenance fixture; never executed.\n")
        self.binary.chmod(0o755)
        self.manifest_path = self.directory / "source-manifest.json"
        self.descriptor_path = self.directory / "base-build.json"
        self.sources = {}
        for relative in WEB_SOURCES:
            source = self.root / relative
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text(f"// Synthetic source-contract fixture: {relative}\n")
            self.sources[relative] = hashlib.sha256(source.read_bytes()).hexdigest()
        self.manifest = dict(self.sources)
        self.descriptor = {
            "head": self.head,
            "status": artifact.PASS_STATUS,
            "checks": [{"name": "synthetic-client-free-evidence", "exit": 0}],
            "backend": "mariadb",
            "profile": "development/TEST_MUD",
            "binary": str(self.binary),
            "binary_sha256": hashlib.sha256(self.binary.read_bytes()).hexdigest(),
            "source_manifest": str(self.manifest_path),
        }
        self.write_evidence()
        environment = patch.dict(os.environ, {
            artifact.BASE_BUILD_ENV: str(self.descriptor_path),
        })
        environment.start()
        self.addCleanup(environment.stop)
        current_head = patch.object(artifact, "_current_head", return_value=self.head)
        self.current_head = current_head.start()
        self.addCleanup(current_head.stop)

    def write_evidence(self):
        self.manifest_path.write_text(json.dumps(self.manifest), encoding="utf-8")
        self.manifest_path.chmod(0o600)
        self.descriptor_path.write_text(json.dumps(self.descriptor), encoding="utf-8")
        self.descriptor_path.chmod(0o600)

    def load(self):
        return artifact.load_base_build(self.root)

    def test_valid_descriptor_checks_actual_bytes_and_checkout_head(self):
        build = self.load()
        self.assertEqual(build.head, self.head)
        self.assertEqual(build.binary, self.binary)
        self.assertEqual(build.binary_sha256, hashlib.sha256(self.binary.read_bytes()).hexdigest())
        self.assertEqual(build.source_manifest, self.manifest_path)
        self.current_head.assert_called_once_with(self.root.resolve())

    def test_missing_configuration_is_not_a_skip(self):
        with patch.dict(os.environ, {artifact.BASE_BUILD_ENV: ""}):
            with self.assertRaisesRegex(RuntimeError, "is required"):
                self.load()

    def test_descriptor_path_rejects_relative_missing_and_symlink(self):
        link = self.directory / "descriptor-link"
        link.symlink_to(self.descriptor_path)
        for candidate in ("base-build.json", str(self.directory / "absent"), str(link)):
            with self.subTest(candidate=candidate), \
                    patch.dict(os.environ, {artifact.BASE_BUILD_ENV: candidate}):
                with self.assertRaises(RuntimeError):
                    self.load()

    def test_descriptor_rejects_non_json_non_object_and_writable_mode(self):
        for raw in ("not JSON", "[]", "null"):
            with self.subTest(raw=raw):
                self.descriptor_path.write_text(raw)
                with self.assertRaises(RuntimeError):
                    self.load()
        self.write_evidence()
        self.descriptor_path.chmod(0o666)
        with self.assertRaisesRegex(RuntimeError, "writable"):
            self.load()

    def test_rejects_foreign_head_and_invalid_build_qualification(self):
        baseline = dict(self.descriptor)
        cases = (
            {"head": "b" * 40}, {"head": "not-a-sha"},
            {"status": "BUILD_FAILED"}, {"backend": "flatfile"},
            {"profile": "production"}, {"checks": []},
            {"checks": [{"exit": False}]}, {"checks": [{"exit": "0"}]},
            {"checks": [{"exit": 1}]}, {"checks": [{}]},
        )
        for updates in cases:
            with self.subTest(updates=updates):
                self.descriptor = {**baseline, **updates}
                self.write_evidence()
                with self.assertRaises(RuntimeError):
                    self.load()

    def test_rejects_binary_drift_unsafe_mode_and_symlink(self):
        self.binary.write_bytes(b"changed bytes")
        with self.assertRaisesRegex(RuntimeError, "SHA-256 mismatch"):
            self.load()
        self.descriptor["binary_sha256"] = hashlib.sha256(self.binary.read_bytes()).hexdigest()
        self.write_evidence()
        for mode in (0o644, 0o775, 0o777):
            with self.subTest(mode=mode):
                self.binary.chmod(mode)
                with self.assertRaises(RuntimeError):
                    self.load()
        self.binary.chmod(0o755)
        link = self.directory / "binary-link"
        link.symlink_to(self.binary)
        self.descriptor["binary"] = str(link)
        self.write_evidence()
        with self.assertRaises(RuntimeError):
            self.load()

    def test_manifest_requires_nonempty_valid_object_and_immutable_metadata(self):
        for raw in ("not JSON", "[]", "{}", '{"src/test.c": "invalid"}'):
            with self.subTest(raw=raw):
                self.manifest_path.write_text(raw)
                with self.assertRaises(RuntimeError):
                    self.load()
        self.write_evidence()
        self.manifest_path.chmod(0o666)
        with self.assertRaisesRegex(RuntimeError, "writable"):
            self.load()

    def test_manifest_rejects_missing_and_changed_source_bytes(self):
        relative = WEB_SOURCES[0]
        source = self.root / relative
        source.write_text("// changed source\n")
        with self.assertRaisesRegex(RuntimeError, "SHA-256 mismatch"):
            self.load()
        source.unlink()
        with self.assertRaisesRegex(RuntimeError, "absent"):
            self.load()

    def test_manifest_rejects_traversal_absolute_and_symlink_sources(self):
        for relative in ("../outside.c", "/outside.c", "src\\outside.c"):
            with self.subTest(relative=relative):
                self.manifest = {relative: "0" * 64}
                self.write_evidence()
                with self.assertRaises(RuntimeError):
                    self.load()
        outside = self.directory / "outside.c"
        outside.write_text("// owned synthetic outside fixture\n")
        link = self.root / "source-link.c"
        link.symlink_to(outside)
        self.manifest = {"source-link.c": hashlib.sha256(outside.read_bytes()).hexdigest()}
        self.write_evidence()
        with self.assertRaises(RuntimeError):
            self.load()

    def test_manifest_never_reads_private_environment_or_git_metadata(self):
        real_hash = artifact._sha256
        for relative in (".env", ".env.local", ".git/config"):
            with self.subTest(relative=relative):
                private = self.root / relative
                private.parent.mkdir(parents=True, exist_ok=True)
                private.write_text("synthetic boundary-test marker, not a real credential\n")
                self.manifest = {
                    **self.sources,
                    relative: hashlib.sha256(private.read_bytes()).hexdigest(),
                }
                self.write_evidence()
                with patch.object(artifact, "_sha256", wraps=real_hash) as hash_input:
                    with self.assertRaisesRegex(RuntimeError, "private|environment"):
                        self.load()
                self.assertNotIn(private, [call.args[0] for call in hash_input.call_args_list])

    def test_web_recovery_preserves_required_authority_source_witnesses(self):
        self.manifest.pop("src/account/account.c")
        self.write_evidence()
        with patch.object(web_fixture, "ROOT", self.root), \
                contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(RuntimeError, "required source"):
                web_fixture.verify_frozen_binary()

    def test_web_recovery_accepts_all_original_authority_source_witnesses(self):
        with patch.object(web_fixture, "ROOT", self.root), \
                contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(web_fixture.verify_frozen_binary(), self.binary)


if __name__ == "__main__":
    unittest.main(verbosity=2)
