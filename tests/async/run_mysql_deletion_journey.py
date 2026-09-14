#!/usr/bin/env python3
"""Real character deletion, transactional refusal and retry on disposable SQL.

Set TEST_DB_HOST (loopback), TEST_DB_USER and TEST_DB_PASSWORD for a disposable
server. No checkout .env or existing schema is used. --server selects a freshly
built MariaDB executable. Only newly created synthetic schemas are touched.
"""
from pathlib import Path
import argparse
import os
import signal
import subprocess
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def run(server):
    database = 'deletion_journey_test_' + uuid.uuid4().hex[:12]
    host = os.environ['TEST_DB_HOST']
    assert host in ('127.0.0.1', 'localhost'), 'use a disposable loopback database'
    environment = {
        'PATH': os.environ.get('PATH', '/usr/bin:/bin'),
        'ENVIRONMENT': 'local', 'DB_HOST': host, 'DB_PORT': '3306',
        'DB_NAME': database, 'DB_USER': os.environ['TEST_DB_USER'],
        'DB_PASSWD': os.environ['TEST_DB_PASSWORD'],
        'DB_ALLOWED_TARGETS': host+'/'+database,
        'MYSQL_PWD': os.environ['TEST_DB_PASSWORD'],
        'PERSISTENCE_MODE': 'mariadb-primary', 'DB_TLS': 'FALSE',
        'REDIS': 'FALSE', 'CHAOS_MUD': 'FALSE', 'DURIS_NEVENT_TRACE_PLAYER': '1',
        'LISTEN_ADDRESS': '127.0.0.1', 'DURIS_WEBSOCKET_LISTEN_ADDRESS': '127.0.0.1',
    }
    if 'LD_LIBRARY_PATH' in os.environ:
        environment['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH']
    mysql = ['mysql', '--protocol=tcp', '-h', host, '-u', environment['DB_USER'], '-N', '-B']

    def sql(text, selected=True):
        return subprocess.check_output(mysql+([database] if selected else []), input=text,
                                       text=True, env=environment).strip()

    def number(text):
        return int(sql(text))

    sql('CREATE DATABASE '+database+' CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci', False)
    try:
        sql((ROOT/'migrations/bootstrap_multithread_safe.sql').read_text())
        subprocess.run(['python3', 'scripts/migration_runner.py', 'adopt', '--kind', 'fresh_bootstrap'],
                       cwd=ROOT, env=environment, check=True)
        subprocess.run(['python3', 'scripts/migration_runner.py', 'run'], cwd=ROOT, env=environment, check=True)
        with tempfile.TemporaryDirectory(prefix='mysql-combat-', dir=ROOT/'bin/tests') as temporary:
            runtime = Path(temporary)
            journey.make_fixture(runtime)
            journey.generate_certificate(runtime)
            (runtime/'logs/log').mkdir(parents=True)
            for name in ('players', 'critical'):
                (runtime/'journals'/name).mkdir(parents=True, mode=0o700)
            plain, tls, websocket = journey.available_ports()
            environment.update(PLAYER_SAVE_JOURNAL_DIR=str(runtime/'journals/players'),
                               CRITICAL_COMMAND_JOURNAL_DIR=str(runtime/'journals/critical'),
                               DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(websocket))
            output_path = runtime/'server.out'
            process = None
            client = None
            with output_path.open('w') as output:
                def boot():
                    offset = output_path.stat().st_size
                    proc = subprocess.Popen([str(server), '--minimal', '-s', '-d', str(runtime), str(plain)],
                                            cwd=runtime, env=environment, stdout=output, stderr=subprocess.STDOUT)
                    deadline = time.monotonic()+120
                    while b'Entering game loop.' not in output_path.read_bytes()[offset:]:
                        if proc.poll() is not None or time.monotonic()>deadline:
                            proc.terminate() if proc.poll() is None else None
                            proc.wait(timeout=10)
                            raise AssertionError('MariaDB combat fixture failed to boot')
                        time.sleep(.1)
                    return proc

                def stop():
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    assert process.returncode == 0

                def choose_delete():
                    client.send('3'); client.expect('Which character do you want to')
                    client.send('1'); client.expect('FINAL WARNING', timeout=30)

                try:
                    process = boot()
                    client = journey.MudClient(plain)
                    journey.create_character(client)
                    client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.', timeout=30)
                    client.send('quit'); client.expect('ACCOUNT MENU', timeout=30)
                    pid = number("SELECT pid FROM player_data WHERE name='"+journey.CHARACTER+"'")
                    items_before = number(f'SELECT COUNT(*) FROM player_items WHERE pid={pid}')
                    assert items_before > 0
                    for label, table, action in [('soft-delete', 'account_characters', 'UPDATE'),
                                                  ('late-cleanup', 'player_data', 'DELETE')]:
                        choose_delete()
                        sql(f"CREATE TRIGGER deletion_fixture_refusal BEFORE {action} ON {table} FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic deletion refusal'")
                        client.send('yes')
                        client.expect('Character deletion did not complete.', timeout=30)
                        client.expect('ACCOUNT MENU')
                        assert b'Character deleted successfully.' not in client.transcript
                        assert number(f'SELECT COUNT(*) FROM player_data WHERE pid={pid}') == 1
                        assert number(f'SELECT COUNT(*) FROM account_characters WHERE pid={pid} AND deleted_at IS NULL') == 1
                        assert number(f'SELECT COUNT(*) FROM player_items WHERE pid={pid}') == items_before
                        sql('DROP TRIGGER deletion_fixture_refusal')
                        client.send('0'); client.close()
                        client = journey.reconnect_character(plain)
                        client.send('inventory'); client.expect('You are carrying')
                        client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.', timeout=30)
                        client.send('quit'); client.expect('ACCOUNT MENU', timeout=30)
                        print(label + ': accurate refusal, mapping/inventory rollback and playable reconnect passed', flush=True)
                    choose_delete(); client.send('yes')
                    client.expect('Character deleted successfully.', timeout=30)
                    client.expect('ACCOUNT MENU')
                    assert number(f'SELECT COUNT(*) FROM player_data WHERE pid={pid}') == 0
                    assert number(f'SELECT COUNT(*) FROM account_characters WHERE pid={pid} AND deleted_at IS NULL') == 0
                    assert client.transcript.count(b'Character deleted successfully.') == 1
                    client.send('3'); client.expect("don't have any characters to delete", timeout=15)
                    client.send('0'); client.close(); client=None
                    stop(); process=boot()
                    # Login only to the account after restart; the deleted player
                    # must not reappear in the selector or authority.
                    client = journey.MudClient(plain)
                    client.expect('Please enter your account name:'); client.send(journey.ACCOUNT)
                    client.expect('Please enter your password:'); client.send(journey.PASSWORD)
                    client.expect('PRESS RETURN'); client.send('')
                    client.expect('ACCOUNT MENU'); client.send('3'); client.expect("don't have any characters to delete")
                    assert number(f'SELECT COUNT(*) FROM player_data WHERE pid={pid}') == 0
                    assert number(f'SELECT COUNT(*) FROM account_characters WHERE pid={pid} AND deleted_at IS NULL') == 0
                    stop()
                    print('successful retry deleted the synthetic character exactly once; account remained usable after restart', flush=True)
                except Exception as error:
                    raise AssertionError(str(error)+'\n'+output_path.read_text(errors='replace')[-10000:]+'\n'+journey.runtime_logs(runtime)) from error
                finally:
                    if client: client.close()
                    if process and process.poll() is None:
                        process.terminate()
                        try: process.wait(timeout=10)
                        except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=10)
    finally:
        sql('DROP DATABASE '+database,False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--server', type=Path, required=True)
    args = parser.parse_args()
    run(args.server.resolve())
