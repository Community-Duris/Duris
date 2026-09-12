#!/usr/bin/env python3
"""Shell entry point with synthetic dump processes; no database is contacted."""
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_persistence_backup import ROOT, policy, provision


class BackupWrapperTests(unittest.TestCase):
    def test_managed_database_dump_success_and_failure(self):
        for advertised in (False, True):
            for mode in ("success", "failure", "invalid", "schema_mismatch"):
                with self.subTest(advertised=advertised, mode=mode), tempfile.TemporaryDirectory(prefix="duris-wrapper-") as temp:
                    base = Path(temp)
                    p = policy(base)
                    provision(base / "live")
                    config = base / "policy.json"
                    config.write_text(json.dumps(p, default=str))
                    config.chmod(0o600)
                    stubs = base / "stubs"
                    stubs.mkdir(mode=0o700)
                    schema = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
                    tables = schema["runtime_table_sql_list"].replace("'", "").split(",")
                    schema["normalized_metadata_fingerprints"]["mariadb10_11"] = hashlib.sha256(b"synthetic-metadata\n").hexdigest()
                    schema_path = base / "synthetic-schema.json"
                    schema_path.write_text(json.dumps(schema, indent=2))
                    schema_path.chmod(0o600)
                    dump = "".join(f"CREATE TABLE `{name}` (synthetic INT);\n" for name in tables)
                    program = '''#!/usr/bin/env python3
import os, sys
from pathlib import Path
if '--help' in sys.argv:
    print('--no-tablespaces' if os.environ['ADVERTISE'] == '1' else '--single-transaction')
    raise SystemExit(0)
assert '--no-defaults' in sys.argv
assert '--single-transaction' in sys.argv
assert '--routines' in sys.argv and '--events' in sys.argv and '--triggers' in sys.argv
assert ('--no-tablespaces' in sys.argv) == (os.environ['ADVERTISE'] == '1')
assert not any('password' in arg.lower() for arg in sys.argv)
if os.environ['DUMP_MODE'] == 'failure':
    raise SystemExit(23)
print(Path(os.environ['SYNTHETIC_DUMP']).read_text() if os.environ['DUMP_MODE'] == 'success' else '-- incomplete dump')
'''
                    (stubs / "mysqldump").write_text(program)
                    (stubs / "mysqldump").chmod(0o700)
                    (stubs / "mysql").write_text('''#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
if '--help' in sys.argv:
    raise SystemExit(0)
query = sys.argv[sys.argv.index('-e') + 1]
schema = json.loads(Path(os.environ['SYNTHETIC_SCHEMA']).read_text())
if query.startswith("SELECT CONCAT('T'"):
    print('mismatched-metadata' if os.environ['DUMP_MODE'] == 'schema_mismatch' else 'synthetic-metadata')
elif 'SELECT VERSION()' in query:
    print('10.11-MariaDB')
elif "engine<>'InnoDB'" in query:
    print(0)
elif 'information_schema.tables' in query:
    print(schema['current_table_count'])
elif 'wallet.pid IS NULL' in query:
    print('0\\t0\\t0\\t0')
else:
    print(1)
''')
                    (stubs / "mysql").chmod(0o700)
                    payload = base / "synthetic.sql"
                    payload.write_text(dump)
                    env = dict(os.environ, PATH=str(stubs) + ":/usr/local/bin:/usr/bin:/bin",
                               BACKUP_ENV_FILE=str(base / "absent.env"), BACKUP_POLICY_FILE=str(config),
                               PERSISTENCE_MODE="mariadb-primary", ENVIRONMENT="local", DB_HOST="localhost",
                               DB_USER="synthetic", DB_PASSWD="synthetic-fixture-only", DB_NAME="synthetic",
                               DB_ALLOWED_TARGETS="localhost/synthetic", DB_PORT="3306", DB_SOCKET="",
                               DUMP_MODE=mode, ADVERTISE="1" if advertised else "0", SYNTHETIC_DUMP=str(payload),
                               SYNTHETIC_SCHEMA=str(schema_path), RUNTIME_COMPATIBILITY_MANIFEST=str(schema_path),
                               PLAYER_SAVE_JOURNAL_DIR=str(base / "journals/players"),
                               CRITICAL_COMMAND_JOURNAL_DIR=str(base / "journals/critical"))
                    result = subprocess.run(["bash", str(ROOT / "scripts/backup_pfiles.sh")], env=env,
                                            capture_output=True, text=True, timeout=30)
                    self.assertEqual(result.returncode == 0, mode == "success", result.stderr)
                    self.assertNotIn("synthetic-fixture-only", result.stdout + result.stderr)
                    generations = list(p["root"].glob("[0-9]*"))
                    self.assertEqual(len(generations), 1 if mode == "success" else 0)
                    self.assertFalse(list(p["root"].glob(".staging-*")))
                    if mode == "success":
                        meta = json.loads((generations[0] / "manifest.json").read_text())
                        self.assertEqual(meta["mode"], "mariadb-primary")
                        self.assertIn("database.sql.gz", meta["files"])
                        with gzip.open(generations[0] / "database.sql.gz", "rt") as stream:
                            self.assertIn("CREATE TABLE `player_data`", stream.read())

    def test_cycle_refuses_boot_on_backup_failure(self):
        cycle = (ROOT / "scripts/cycle_mud.sh").read_text()
        self.assertIn("./scripts/backup_pfiles.sh", cycle)
        self.assertIn("job_overlap_or_authority_busy", cycle)
        self.assertIn('echo "Required $PERSISTENCE_MODE backup failed; refusing to boot"', cycle)


if __name__ == "__main__":
    unittest.main()
