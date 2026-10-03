#!/usr/bin/env python3
"""Real whole-account alias erasure and refusal/recovery on disposable SQL.

Uses only freshly created synthetic schemas on an explicitly selected loopback
server. No checkout .env, existing schema, or live game is used.
"""
from pathlib import Path
import argparse
import os
import re
import signal
import subprocess
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def run(server):
    # The unchanged native exclusion prefix plus this schema must fit MySQL's
    # 64-byte named-lock limit (MariaDB accepts the older 65-byte fixture name).
    database = 'deletion_test_' + uuid.uuid4().hex[:12]
    host = os.environ['TEST_DB_HOST']
    assert host in ('127.0.0.1', 'localhost'), 'use a disposable loopback database'
    port_text = os.environ.get('TEST_DB_PORT', '3306')
    if not re.fullmatch(r'[0-9]{1,5}', port_text) or not 1 <= int(port_text) <= 65535:
        raise RuntimeError('TEST_DB_PORT must be a TCP port from 1 to 65535')
    port = str(int(port_text))
    environment = {
        'PATH': os.environ.get('PATH', '/usr/bin:/bin'),
        'ENVIRONMENT': 'local', 'DB_HOST': host, 'DB_PORT': port,
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
    mysql = ['mysql', '--protocol=tcp', '-h', host, '-P', port,
             '-u', environment['DB_USER'], '-N', '-B']

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
                    # Capture native creation's personal alias, plus another season
                    # and a distinct PID. Reload the service before fault injection.
                    revision = number('SELECT catalog_revision FROM zone_story_quest_state WHERE state_id=1')
                    state = bytes.fromhex(sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1')).decode()
                    assert f'N|1|{pid}|' in state
                    state += f'N|7|{pid}|416e6369656e74|1\nN|1|999|556e72656c61746564|1\n'
                    sql('UPDATE zone_story_quest_state SET state_blob=0x'+state.encode().hex()+' WHERE state_id=1')
                    client.close(); client=None; stop(); process=boot()
                    client = journey.reconnect_character(plain)
                    client.send('quit'); client.expect('ACCOUNT MENU', timeout=30)
                    original = sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1')
                    def unchanged():
                        assert number(f'SELECT COUNT(*) FROM player_data WHERE pid={pid}') == 1
                        assert number(f'SELECT COUNT(*) FROM player_items WHERE pid={pid}') == items_before
                        assert number("SELECT blocked FROM accounts WHERE account_name='"+journey.ACCOUNT+"'") == 2
                    def retry():
                        client.send(journey.ACCOUNT)
                    client.send('7'); client.expect('Re-enter your account password')
                    client.send(journey.PASSWORD); client.expect('PERMANENT ACCOUNT DELETION', timeout=30)
                    client.expect('CANCEL:')
                    # Corrupt state and a stale catalog refuse before destructive
                    # erasure. The permanent fence remains non-cancellable.
                    for label, change in (
                            ('malformed-state', "state_blob='invalid-personal-state'"),
                            ('stale-catalog', f'catalog_revision={revision+1}')):
                        sql('UPDATE zone_story_quest_state SET '+change+' WHERE state_id=1')
                        damaged = sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1')
                        retry(); client.expect('Account deletion did not complete.', timeout=30)
                        client.expect('to retry completion:')
                        unchanged()
                        assert sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1') == damaged
                        sql(f'UPDATE zone_story_quest_state SET catalog_revision={revision},state_blob=0x'+original+' WHERE state_id=1')
                        print(label+': native refusal preserved identities, items and exact quest state',flush=True)
                    # Refusing the quest-state write also preserves the native
                    # deletion boundary and exact original state.
                    sql("CREATE TRIGGER quest_alias_refusal BEFORE UPDATE ON zone_story_quest_state FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic quest erasure refusal'")
                    retry(); client.expect('Account deletion did not complete.', timeout=30)
                    client.expect('to retry completion:'); unchanged()
                    assert sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1') == original
                    sql('DROP TRIGGER quest_alias_refusal')
                    print('quest write failure: native deletion refused without changing authority',flush=True)
                    # A late native failure must roll back the earlier alias rewrite.
                    sql("CREATE TRIGGER account_alias_refusal BEFORE DELETE ON player_data FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic account erasure refusal'")
                    retry(); client.expect('Account deletion did not complete.', timeout=30)
                    client.expect('to retry completion:'); unchanged()
                    assert sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1') == original
                    sql('DROP TRIGGER account_alias_refusal')
                    print('late native failure: aliases and player state rolled back together',flush=True)
                    retry(); client.expect('Your account and all of its characters were permanently deleted.', timeout=30)
                    assert client.transcript.count(b'were permanently deleted.') == 1
                    assert number(f'SELECT COUNT(*) FROM player_data WHERE pid={pid}') == 0
                    assert number("SELECT COUNT(*) FROM accounts WHERE account_name='"+journey.ACCOUNT+"'") == 0
                    def erased():
                        encoded = bytes.fromhex(sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1')).decode()
                        aliases = [row.split('|') for row in encoded.splitlines() if row.startswith('N|')]
                        assert not any(int(row[2]) == pid for row in aliases), 'deleted PID retained a personal alias'
                        assert any(int(row[2]) == 999 for row in aliases), 'unrelated PID alias was erased'
                        assert f'X|1|{pid}' in encoded and f'X|7|{pid}' in encoded, 'nonpersonal erasure markers were lost'
                    erased(); client.close(); client=None
                    # A later same-process state write exercises cache invalidation.
                    client=journey.MudClient(plain)
                    journey.create_character(client, account='Eraseview', character='Observer')
                    client.send('save'); client.expect('Save complete for Observer.', timeout=30)
                    published = bytes.fromhex(sql('SELECT HEX(state_blob) FROM zone_story_quest_state WHERE state_id=1')).decode()
                    new_pid = number("SELECT pid FROM player_data WHERE name='Observer'")
                    assert new_pid != pid
                    assert sum(row.startswith(f'N|1|{new_pid}|4f62736572766572|') for row in published.splitlines()) == 1, \
                        'post-erasure cache refresh disabled quest publication'
                    client.send('quit'); client.expect('ACCOUNT MENU',timeout=30)
                    erased(); client.close(); client=None; stop(); process=boot()
                    client=journey.MudClient(plain)
                    client.expect('Please enter your account name:'); client.send(journey.ACCOUNT)
                    client.expect('is this correct?',timeout=30)
                    erased()
                    assert number(f'SELECT COUNT(*) FROM player_data WHERE pid={pid}') == 0
                    assert number("SELECT COUNT(*) FROM accounts WHERE account_name='"+journey.ACCOUNT+"'") == 0
                    stop()
                    print('SQL whole-account alias erasure, rollback, repaired retry, cache write and cold restart passed',flush=True)
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
