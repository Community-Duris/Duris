"""Independent diagnostic fixture; native mutation is confined to fresh test DBs.

No shared source is edited. The stderr tap preserves the real mysql process,
stdin/stdout, flags, SQL bytes, session and runner decisions. This is an isolated
fixture, never an operational audit or schema/release qualification.
"""
import argparse
import contextlib
from dataclasses import replace
import hashlib
import io
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

HEAD = '0065_zone_reset_item_birth_origin'
CLI_REFUSAL = ('migration runner blocked: migration ' + HEAD +
               ' apply failed: migration SQL command failed or session lost\n')
MAX_ERROR_BYTES = 65536
REAL_POPEN = subprocess.Popen

def expected_refusal(cli_code, cli_error, raw_error):
    return (len(raw_error.encode('utf-8')) <= MAX_ERROR_BYTES and cli_code == 2 and cli_error == CLI_REFUSAL and
        re.fullmatch(r'(?:--------------\r?\nCALL duris_verify_zone_reset_item_origin\(\);?\r?\n'
                     r'--------------\r?\n(?:\r?\n)?)?'
                     r'ERROR 1644 \(45000\) at line [1-9][0-9]*: incompatible zone reset item '
                     r'(?:origin|terminal) schema\r?\n', raw_error) is not None)

class DiagnosticPredicateTests(unittest.TestCase):
    def test_actual_error_frame_required(self):
        for message in ('origin', 'terminal'):
            raw = f'ERROR 1644 (45000) at line 195: incompatible zone reset item {message} schema\n'
            self.assertTrue(expected_refusal(2, CLI_REFUSAL, raw))
            frame = '--------------\nCALL duris_verify_zone_reset_item_origin()\n--------------\n\n'
            self.assertTrue(expected_refusal(2, CLI_REFUSAL, frame + raw))
            self.assertFalse(expected_refusal(2, CLI_REFUSAL, frame.replace('origin()', 'unrelated()') + raw))
            self.assertFalse(expected_refusal(0, CLI_REFUSAL, raw))
            self.assertFalse(expected_refusal(2, CLI_REFUSAL.replace('apply failed', 'verify failed'), raw))

    def test_transport_and_other_sql_errors_refused(self):
        for raw in ('', 'ERROR 2013 (HY000) at line 1: Lost connection to server during query\n',
                    'ERROR 1146 (42S02) at line 1: Table diag.zone_reset_item does not exist\n',
                    'ERROR 1644 (45000) at line 1: unrelated zone reset item failure\n',
                    'ERROR 1644 (45000) at line 1: incompatible zone reset item terminal schema!\n'):
            self.assertFalse(expected_refusal(2, CLI_REFUSAL, raw), raw)

    def test_extra_error_output_refused(self):
        raw = 'ERROR 1644 (45000) at line 195: incompatible zone reset item terminal schema\n'
        self.assertFalse(expected_refusal(2, CLI_REFUSAL, 'warning\n' + raw))
        self.assertFalse(expected_refusal(2, CLI_REFUSAL, raw + 'ERROR 2013 (HY000)\n'))
        self.assertFalse(expected_refusal(2, CLI_REFUSAL, raw.replace('195', '1' * MAX_ERROR_BYTES)))

class StderrTap:
    def __init__(self, directory, evidence, expected_command):
        self.directory = directory
        self.evidence = evidence
        self.expected_command = list(expected_command)
        self.streams = []
        self.clients = []

    def spawn(self, command, *args, **kwargs):
        if (isinstance(command, list) and command[:2] == ['mysql', '--no-defaults'] and
                '--skip-reconnect' in command and kwargs.get('stderr') == subprocess.DEVNULL):
            assert command == self.expected_command, 'unexpected persistent mysql invocation'
            assert kwargs['stdin'] == kwargs['stdout'] == subprocess.PIPE and kwargs['bufsize'] == 0
            assert '--unbuffered' in command and '--default-character-set=utf8mb4' in command
            path = self.directory / f'client-{len(self.clients) + 1}.stderr.txt'
            fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
            stream = os.fdopen(fd, 'wb', buffering=0)
            self.streams.append((path, stream))
            kwargs = dict(kwargs, stderr=stream)
            client = REAL_POPEN(command, *args, **kwargs)
            self.clients.append(client)
            return client
        return REAL_POPEN(command, *args, **kwargs)

    def __enter__(self):
        self.directory.mkdir(mode=0o700)
        self.evidence.mkdir()
        self.patch = mock.patch.object(subprocess, 'Popen', self.spawn)
        self.patch.start()
        return self

    def __exit__(self, *exc):
        self.patch.stop()
        for path, stream in self.streams:
            stream.close()
        for path, stream in self.streams:
            with path.open('rb') as captured:
                payload = captured.read(MAX_ERROR_BYTES + 1)
            copied = self.evidence / path.name
            with copied.open('xb') as destination:
                destination.write(payload)
            metadata = dict(private_capture_path=str(path), private_capture_mode=oct(path.stat().st_mode & 0o777),
                original_bytes=path.stat().st_size, bounded_copied_bytes=len(payload),
                bounded_copy_sha256=hashlib.sha256(payload).hexdigest(),
                copy_is_prefix_only=path.stat().st_size > MAX_ERROR_BYTES)
            (self.evidence / (path.name + '.metadata.json')).write_text(json.dumps(metadata, indent=2) + '\n')
            assert path.stat().st_size <= MAX_ERROR_BYTES, 'client diagnostic exceeds bound'

    def result(self):
        assert len(self.clients) == len(self.streams) == 1, 'expected one actual persistent client'
        client = self.clients[0]
        assert client.poll() is not None, 'diagnostic leaked a persistent mysql client'
        path = self.streams[0][0]
        assert path.stat().st_mode & 0o777 == 0o600
        with path.open('rb') as stream:
            payload = stream.read(MAX_ERROR_BYTES + 1)
        assert len(payload) <= MAX_ERROR_BYTES, 'client diagnostic read exceeds bound'
        return dict(client_exit=client.returncode, raw_error=payload.decode('utf-8'),
                    sha256=hashlib.sha256(payload).hexdigest(), bytes=len(payload), private_capture_mode='0600')

def native(args):
    assert sys.platform == 'linux'
    assert (os.environ.get('ENVIRONMENT'), os.environ.get('TEST_DB_DISPOSABLE'),
        os.environ.get('STAGING_FORK_DISPOSABLE_SERVER'), os.environ.get('DB_HOST'),
        os.environ.get('DB_PORT')) == ('test', '1', '1', '127.0.0.1', '3306')
    assert os.environ.get('DB_USER') == 'root' and os.environ.get('DB_PASSWD')
    assert not os.environ.get('DB_SOCKET') and os.environ.get('DURIS_DIAGNOSTIC_JOB') == 'ACC-02-ORIGINAL65-DIAG-02'
    assert re.fullmatch('[a-f0-9]{10}', args.token)
    root = args.source.resolve()
    out = args.output.resolve()
    assert str(root) == '/opt/duris' and str(out).startswith('/plan5-diagnostic/')
    out.mkdir(mode=0o700, parents=True, exist_ok=True)
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/async')]
    import migration_runner as runner
    import test_staging_migration_fork_mysql as matrix
    assert hashlib.sha256((root / 'scripts/migration_runner.py').read_bytes()).hexdigest() == \
        'e259afab67a900ce26d735abf4c3fa501e14f14b7baf0dfaff8b5317e23cfe92'
    original = runner.load_manifest(root / 'migrations/migration_manifest.json')
    variant = runner.load_manifest(root / 'migrations/migration_manifest.nullable_default_0065.json')
    assert len(original.migrations) == len(variant.migrations) == 65
    assert original.migrations[-1].migration_id == HEAD
    assert variant.migrations[-1].migration_id == HEAD + '_nullable_default'
    assert original.migrations[:64] == variant.migrations[:64]
    prefix = replace(original, migrations=original.migrations[:64])
    runtime = json.loads((root / 'migrations/runtime_compatibility_manifest.json').read_text())
    clients = []
    engines = []
    records = []
    private_stderr = tempfile.TemporaryDirectory(prefix='plan5-schema65-stderr-', dir='/tmp')
    capture = Path(private_stderr.name)
    assert capture.stat().st_mode & 0o777 == 0o700

    def settings(engine):
        os.environ.update(engine.environment())

    def snapshot(engine):
        return dict(history=matrix.history(engine, 64),
            state=engine.sql('SELECT applied_count,LOWER(HEX(history_checksum)) FROM mud_schema_migration_state WHERE state_id=1;', database=engine.database),
            protected=matrix.description_rows(engine),
            tail_count=engine.sql('SELECT COUNT(*) FROM mud_schema_history WHERE sequence_number>=65;', database=engine.database))

    def setup(tag):
        engine = matrix.LoopbackEngine('mariadb10_11', '', os.environ['DB_PASSWD'],
                                      f'duris_268_{args.token}_diag{tag}test')
        engines.append(engine)
        assert engine.sql('SELECT COUNT(*) FROM information_schema.schemata WHERE schema_name=' +
                          "'" + engine.database + "';") == '0', 'existing diagnostic namespace refused'
        matrix.setup(engine, original)
        assert engine.server_version == '10.11.19-MariaDB-ubu2204'
        settings(engine)
        executor = runner.MysqlExecutor(prefix)
        clients.append(executor)
        assert len(runner.run_pending(prefix, executor)) == 64
        executor.release_lock()
        matrix.descriptions(engine)
        before = snapshot(engine)
        assert before['state'] == '64\t' + runtime['migration_head']['history_checksum']
        assert before['tail_count'] == '0'
        return engine, before

    def original_cli(engine):
        settings(engine)
        previous = sys.argv
        sys.argv = ['scripts/migration_runner.py', '--manifest',
                    str(root / 'migrations/migration_manifest.json'), 'run']
        output, error = io.StringIO(), io.StringIO()
        code = 0
        try:
            with contextlib.redirect_stdout(output), contextlib.redirect_stderr(error):
                runpy.run_path(str(root / 'scripts/migration_runner.py'), run_name='__main__')
        except SystemExit as ended:
            code = ended.code
        finally:
            sys.argv = previous
        return dict(cli_exit=code, cli_stdout=output.getvalue(), cli_stderr=error.getvalue())

    try:
        for tag in ('kill', 'sql', 'signal'):
            engine, before = setup(tag)
            settings(engine)
            executor = runner.MysqlExecutor(original)
            clients.append(executor)
            apply = executor.apply

            def fault(step):
                assert step.migration_id == HEAD
                if tag == 'kill':
                    connection = executor.sql('SELECT CONNECTION_ID();')
                    engine.sql('KILL CONNECTION ' + connection + ';')
                    apply(step)
                elif tag == 'sql':
                    executor.sql('SELECT * FROM nonexistent_diag_zone_reset_item;')
                else:
                    executor.sql("SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unrelated zone reset item failure';")

            with StderrTap(capture / tag, out / tag, executor.command) as tap:
                with mock.patch.object(executor, 'apply', side_effect=fault):
                    try:
                        runner.run_pending(original, executor)
                    except runner.MigrationContractError as error:
                        refused = str(error)
                    else:
                        raise AssertionError('native fault was accepted: ' + tag)
            raw = tap.result()
            assert not expected_refusal(2, 'migration runner blocked: ' + refused + '\n', raw['raw_error'])
            assert executor._client is None and executor._session_closed
            try:
                executor.sql('SELECT 1;')
            except runner.MigrationContractError as error:
                assert str(error) == 'migration SQL session is closed'
            else:
                raise AssertionError('failed executor reconnected')
            assert snapshot(engine) == before
            if tag == 'sql':
                assert re.fullmatch(r'(?:--------------\nSELECT \* FROM nonexistent_diag_zone_reset_item\n--------------\n\n?)?'
                    r'ERROR 1146 \(42S02\) at line [1-9][0-9]*: .*nonexistent_diag_zone_reset_item.*\n', raw['raw_error'])
            elif tag == 'signal':
                assert re.fullmatch(r"(?:--------------\nSIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unrelated zone reset item failure'\n--------------\n\n?)?"
                    r'ERROR 1644 \(45000\) at line [1-9][0-9]*: unrelated zone reset item failure\n', raw['raw_error'])
            records.append(dict(control=tag, database=engine.database, refusal=refused,
                diagnostic=raw, intended_schema_refusal=False, history_state_protected_rows_unchanged=True,
                receipt65_count=0, failed_client_closed_no_reconnect=True))

        engine, before = setup('original')
        settings(engine)
        command = runner.MysqlExecutor(original).command
        with StderrTap(capture / 'original', out / 'original', command) as tap:
            result = original_cli(engine)
        result['diagnostic'] = tap.result()
        assert snapshot(engine) == before, 'original refusal changed64 receipts/state/protected rows or recorded65'
        result['history_state_protected_rows_unchanged'] = True
        result['receipt65_count'] = 0
        result['intended_schema_refusal'] = expected_refusal(result['cli_exit'], result['cli_stderr'],
                                                            result['diagnostic']['raw_error'])
        (out / 'positive-refusal.json').write_text(json.dumps(result, indent=2) + '\n')
        assert result['intended_schema_refusal'], 'actual original65 cause is outside the exact intended SQL error'
        result['terminal_default_metadata'] = engine.sql('SELECT is_nullable,IF(column_default IS NULL,\'SQL_NULL\',HEX(column_default)) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name=\'zone_reset_item_birth_origin\' AND column_name=\'terminal_publication_context\';', database=engine.database)
        settings(engine)
        executor = runner.MysqlExecutor(variant)
        clients.append(executor)
        assert runner.run_pending(variant, executor) == [HEAD + '_nullable_default']
        executor.release_lock()
        assert snapshot(engine)['history'] == before['history']
        assert snapshot(engine)['protected'] == before['protected']
        assert snapshot(engine)['tail_count'] == '1'
        matrix.require_schema65_history(engine, 'nullable_default_migration_head', runtime['schema65'])
        receipts65 = matrix.history(engine)
        state65 = snapshot(engine)['state']
        rerun = runner.MysqlExecutor(variant)
        clients.append(rerun)
        assert runner.run_pending(variant, rerun) == []
        rerun.release_lock()
        assert matrix.history(engine) == receipts65 and snapshot(engine)['state'] == state65
        wrong = matrix.migrate(engine, 'migration_manifest.json', False)
        assert 'applied migration history was edited or reordered' in wrong
        assert matrix.history(engine) == receipts65 and snapshot(engine)['state'] == state65
        result.update(nullable_recovery_rerun_wrong_selector='PASS', original64_receipts_preserved=True)
        records.append(dict(control='original', database=engine.database, result=result))
        (out / 'diagnostic-result.json').write_text(json.dumps(dict(status='PASS_DIAGNOSTIC_ONLY',
            source='6bfa0f7ebdd5f2f46841047fdd10522e4d27dfea', server_version=engine.server_version,
            cases=records, qualification='UNQUALIFIED', schema65_boot_report_contract_writes=False,
            accounting='INACTIVE', admission='CLOSED'), indent=2) + '\n')
        print('PASS_DIAGNOSTIC_ONLY: three native transport/SQL negatives and exact original65 error/history/recovery controls')
    finally:
        for client in clients:
            client.release_lock()
        for engine in engines:
            engine.directory.cleanup()
        private_stderr.cleanup()
        (out / 'native-control-results.json').write_text(json.dumps(records, indent=2) + '\n')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-only', action='store_true')
    parser.add_argument('--native', action='store_true')
    parser.add_argument('--source', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--token')
    args = parser.parse_args()
    if args.check_only and not args.native:
        result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(DiagnosticPredicateTests))
        raise SystemExit(0 if result.wasSuccessful() else 1)
    assert args.native and not args.check_only and args.source and args.output and args.token
    native(args)
