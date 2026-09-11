#!/usr/bin/env python3
"""Run the maintained importer against a disposable MariaDB schema, never game data."""
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import uuid

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(os.environ.get('TEST_DB_HOST'), 'requires disposable TEST_DB_HOST')
class AtomicImport(unittest.TestCase):
    def test_rollback_and_consistent_publication(self):
        schema = 'help_import_test_' + uuid.uuid4().hex[:12]
        env = dict(os.environ, MYSQL_PWD='cache-test', DB_PASSWD='cache-test',
                   DB_HOST=os.environ['TEST_DB_HOST'], DB_PORT='3306', DB_USER='root',
                   DB_NAME=schema, DB_SOCKET='')
        mysql = ['mysql', '-h', env['DB_HOST'], '-uroot', '-N', '-B']

        def query(sql, database=True):
            return subprocess.run(mysql + ([schema] if database else []), input=sql,
                                  text=True, env=env, capture_output=True, check=True).stdout.strip()

        query(f'CREATE DATABASE {schema}', False)
        try:
            query("CREATE TABLE pages (title VARCHAR(255),text MEDIUMTEXT,category_id INT,"
                  "last_update DATETIME,last_update_by VARCHAR(255), CHECK(title<>'broken')) ENGINE=InnoDB;"
                  "CREATE TABLE mud_info (name VARCHAR(255) PRIMARY KEY,content MEDIUMTEXT) ENGINE=InnoDB;"
                  "INSERT INTO pages VALUES ('help','original',0,NOW(),'test'),('sentinel','old',0,NOW(),'test');"
                  "INSERT INTO mud_info VALUES ('news','original news');")
            with tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                (root / 'scripts').mkdir()
                (root / 'lib/information').mkdir(parents=True)
                (root / 'help').mkdir()
                script = root / 'scripts/import_help_to_prod.sh'
                script.write_text((ROOT / 'scripts/import_help_to_prod.sh').read_text())
                (root / 'lib/information/help').write_text('new help')
                (root / 'lib/information/news').write_text('new news')
                index = root / 'lib/information/help_index'
                index.write_text('#\n"broken"\nrejected test entry\n')
                (root / 'help/duris_help_parsed.hlp').write_text(
                    'Fresh\nFresh - Last Edited: 2026-09-11 by tester\na complete fresh page\n')
                failed = subprocess.run(['bash', str(script), '--clean'], input='yes\nyes\n',
                                        text=True, env=env, capture_output=True, timeout=30)
                self.assertNotEqual(failed.returncode, 0, failed.stdout)
                self.assertEqual(query('SELECT GROUP_CONCAT(title ORDER BY title) FROM pages'), 'help,sentinel')
                self.assertEqual(query("SELECT content FROM mud_info WHERE name='news'"), 'original news')

                index.write_text('#\n"Alias"\na new alias entry\n')
                query('CREATE TRIGGER slow_import BEFORE INSERT ON pages FOR EACH ROW SET @delay=SLEEP(0.4)')
                process = subprocess.Popen(['bash', str(script), '--clean'], stdin=subprocess.PIPE,
                                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
                process.stdin.write('yes\nyes\n')
                process.stdin.close()
                snapshots = []
                deadline = time.monotonic() + 30
                try:
                    while process.poll() is None:
                        self.assertLess(time.monotonic(), deadline)
                        snapshots.append(query('SELECT GROUP_CONCAT(title ORDER BY title) FROM pages'))
                        time.sleep(0.03)
                    self.assertEqual(process.returncode, 0, process.stderr.read())
                    final = query('SELECT GROUP_CONCAT(title ORDER BY title) FROM pages')
                    self.assertEqual(final, 'Alias,Fresh,help')
                    self.assertIn('help,sentinel', snapshots)
                    self.assertTrue(set(snapshots) <= {'help,sentinel', final}, snapshots)
                    self.assertEqual(query("SELECT content FROM mud_info WHERE name='news'"), 'new news')
                finally:
                    if process.poll() is None:
                        process.kill()
                    process.wait()
                    process.stdout.close()
                    process.stderr.close()
                # Refuse nontransactional destinations before staging deletes.
                query('DROP TRIGGER slow_import; ALTER TABLE mud_info MODIFY name VARCHAR(128) NOT NULL, ENGINE=MyISAM')
                refused = subprocess.run(['bash', str(script)], input='yes\n', env=env,
                                         text=True, capture_output=True, timeout=30)
                self.assertNotEqual(refused.returncode, 0)
                self.assertIn('requires existing InnoDB', refused.stdout)
                self.assertEqual(query('SELECT GROUP_CONCAT(title ORDER BY title) FROM pages'), final)
        finally:
            query(f'DROP DATABASE {schema}', False)


if __name__ == '__main__':
    unittest.main()
