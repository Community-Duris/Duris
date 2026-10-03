#!/usr/bin/env python3
"""Linux restore socket bounds refuse before provisioning a disposable daemon."""
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(sys.platform.startswith("linux"), "native restore is Linux-only")
class RestoreSocketBounds(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sys.path.insert(0, str(ROOT / "scripts"))
        import persistence_restore
        cls.restore = persistence_restore

    def candidate(self, root, socket_bytes):
        suffix_bytes = len(os.fsencode(root / "" / "mysql.sock")) + 1
        candidate = root / ("a" * (socket_bytes - suffix_bytes))
        candidate.mkdir(mode=0o700)
        self.assertEqual(len(os.fsencode(candidate / "mysql.sock")), socket_bytes)
        return candidate

    def test_overlong_socket_refuses_before_files_or_processes(self):
        with tempfile.TemporaryDirectory(prefix="rs-", dir="/tmp") as temporary:
            candidate = self.candidate(Path(temporary), 108)
            with mock.patch.object(self.restore.backup, "run") as run, \
                    mock.patch.object(self.restore.subprocess, "Popen") as start:
                with self.assertRaisesRegex(self.restore.backup.BackupError,
                                            "isolated_database_socket_path_too_long"):
                    with self.restore.private_database(candidate):
                        self.fail("overlong socket admitted")
            run.assert_not_called()
            start.assert_not_called()
            self.assertEqual(list(candidate.iterdir()), [])

    def test_limit_is_bytes_and_not_characters(self):
        with tempfile.TemporaryDirectory(prefix="rs-", dir="/tmp") as temporary:
            candidate = self.candidate(Path(temporary), 107)
            candidate.rmdir()
            candidate = candidate.with_name(candidate.name[:-2] + "éé")
            candidate.mkdir(mode=0o700)
            self.assertEqual(len(str(candidate / "mysql.sock")), 107)
            self.assertEqual(len(os.fsencode(candidate / "mysql.sock")), 109)
            with mock.patch.object(self.restore.backup, "run") as run:
                with self.assertRaisesRegex(self.restore.backup.BackupError,
                                            "isolated_database_socket_path_too_long"):
                    with self.restore.private_database(candidate):
                        self.fail("overlong multibyte socket admitted")
            run.assert_not_called()
            self.assertEqual(list(candidate.iterdir()), [])

    def test_unknown_engine_refuses_before_files_or_processes(self):
        with tempfile.TemporaryDirectory(prefix="re-", dir="/tmp") as temporary:
            candidate = Path(temporary)
            with mock.patch.object(self.restore.backup, "run") as run, \
                    mock.patch.object(self.restore.subprocess, "Popen") as start:
                with self.assertRaisesRegex(self.restore.backup.BackupError,
                                            "invalid_restore_database_engine"):
                    with self.restore.private_database(candidate, "unknown"):
                        self.fail("unknown engine admitted")
            run.assert_not_called()
            start.assert_not_called()
            self.assertEqual(list(candidate.iterdir()), [])

    def test_mysql_choice_refuses_mariadb_binary_before_initialization(self):
        with tempfile.TemporaryDirectory(prefix="re-", dir="/tmp") as temporary:
            candidate = Path(temporary)
            with mock.patch.object(self.restore.shutil, "which", return_value="/usr/sbin/mysqld"), \
                    mock.patch.object(self.restore.backup, "run",
                                      return_value=b"mysqld Ver 10.11.14-MariaDB") as run, \
                    mock.patch.object(self.restore.subprocess, "Popen") as start:
                with self.assertRaisesRegex(self.restore.backup.BackupError,
                                            "mysql_restore_version_unsupported"):
                    with self.restore.private_database(candidate, "mysql"):
                        self.fail("MariaDB binary admitted for MySQL")
            run.assert_called_once()
            start.assert_not_called()
            self.assertFalse((candidate / "mysql").exists())

    def test_mysql_choice_requires_an_installed_executable(self):
        with tempfile.TemporaryDirectory(prefix="re-", dir="/tmp") as temporary:
            candidate = Path(temporary)
            with mock.patch.object(self.restore.shutil, "which", return_value=None), \
                    mock.patch.object(self.restore.backup, "run") as run, \
                    mock.patch.object(self.restore.subprocess, "Popen") as start:
                with self.assertRaisesRegex(self.restore.backup.BackupError,
                                            "mysql_restore_executable_required"):
                    with self.restore.private_database(candidate, "mysql"):
                        self.fail("missing MySQL binary admitted")
            run.assert_not_called()
            start.assert_not_called()
            self.assertFalse((candidate / "mysql").exists())

    def test_mysql8_initialization_uses_resolved_binary_and_basedir(self):
        class InitializationReached(Exception):
            pass
        with tempfile.TemporaryDirectory(prefix="re-", dir="/tmp") as temporary:
            candidate = Path(temporary)
            executable = "/opt/disposable-mysql/bin/mysqld"
            with mock.patch.object(self.restore.shutil, "which", return_value=executable), \
                    mock.patch.object(self.restore.backup, "run", side_effect=[
                        b"mysqld Ver 8.0.46 for Linux", InitializationReached]) as run:
                with self.assertRaises(InitializationReached):
                    with self.restore.private_database(candidate, "mysql"):
                        self.fail("mock initialization should stop the probe")
            self.assertEqual(run.call_count, 2)
            args = run.call_args.args[0]
            self.assertEqual(args[0], executable)
            self.assertIn("--initialize-insecure", args)
            self.assertIn("--basedir=/opt/disposable-mysql", args)
            self.assertIn("--datadir=" + str(candidate / "mysql"), args)
            self.assertIn("--innodb-use-native-aio=OFF", args)
            self.assertIn("--mysqlx=OFF", args)

    def test_last_supported_byte_reaches_initialization(self):
        class InitializationReached(Exception):
            pass
        with tempfile.TemporaryDirectory(prefix="rs-", dir="/tmp") as temporary:
            candidate = self.candidate(Path(temporary), 107)
            with mock.patch.object(self.restore.backup, "run",
                                   side_effect=InitializationReached) as run:
                with self.assertRaises(InitializationReached):
                    with self.restore.private_database(candidate):
                        self.fail("mock initialization should stop the probe")
            run.assert_called_once()
            self.assertEqual(run.call_args.args[0][0], "mariadb-install-db")
            self.assertIn("--innodb-use-native-aio=OFF", run.call_args.args[0])


if __name__ == "__main__":
    unittest.main()
