#!/usr/bin/env python3
"""Carry finite ward state through real SQL saves, cold loads, and exec copyover.

Use an absolute MariaDB server executable and the disposable TEST_DB_* settings
documented by test_mysql_combat_journey.py. No checkout .env is used.
"""
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def run(binary):
    assert os.environ.get('TEST_DB_DISPOSABLE') == '1', 'TEST_DB_DISPOSABLE=1 is required'
    host = os.environ['TEST_DB_HOST']
    assert host in ('127.0.0.1', 'localhost'), 'use a disposable loopback database'
    database = 'ward_journey_test_' + uuid.uuid4().hex[:12]
    env = dict(PATH=os.environ.get('PATH', '/usr/bin:/bin'), ENVIRONMENT='local',
               DB_HOST=host, DB_PORT=os.environ.get('TEST_DB_PORT', '3306'),
               DB_USER=os.environ['TEST_DB_USER'], DB_PASSWD=os.environ['TEST_DB_PASSWORD'],
               MYSQL_PWD=os.environ['TEST_DB_PASSWORD'], DB_NAME=database,
               DB_ALLOWED_TARGETS=host+'/'+database, PERSISTENCE_MODE='mariadb-primary',
               DB_TLS='FALSE', REDIS='FALSE', CHAOS_MUD='FALSE', LISTEN_ADDRESS='127.0.0.1',
                   DURIS_WEBSOCKET_LISTEN_ADDRESS='127.0.0.1', DURIS_NEVENT_TRACE_PLAYER='1')
    if os.environ.get('LD_LIBRARY_PATH'):
        env['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH']
    mysql = ['mysql', '--protocol=tcp', '-h', host, '-P', env['DB_PORT'],
             '-u', env['DB_USER'], '-N', '-B']

    def sql(query, selected=True):
        return subprocess.check_output(mysql+([database] if selected else []), input=query,
                                       text=True, env=env).strip()

    sql('CREATE DATABASE '+database+' CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci', False)
    process = client = None
    try:
        sql((ROOT/'migrations/bootstrap_multithread_safe.sql').read_text())
        for action in (['adopt', '--kind', 'fresh_bootstrap'], ['run']):
            subprocess.run(['python3', 'scripts/migration_runner.py', *action],
                           cwd=ROOT, env=env, check=True)
        with tempfile.TemporaryDirectory(prefix='duris-ward-persistence-') as temporary:
            runtime = Path(temporary)
            journey.make_fixture(runtime)
            journey.generate_certificate(runtime)
            (runtime/'Players').mkdir(mode=0o700)
            (runtime/'logs/log').mkdir(parents=True)
            (runtime/'copyover').mkdir()
            (runtime/'bin/server').mkdir(parents=True)
            for name in ('dms', 'dms_new'):
                shutil.copy2(binary, runtime/'bin/server'/name)
            for name in ('players', 'critical'):
                (runtime/'journals'/name).mkdir(parents=True, mode=0o700)
            plain, tls, websocket = journey.available_ports()
            env.update(PLAYER_SAVE_JOURNAL_DIR=str(runtime/'journals/players'),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(runtime/'journals/critical'),
                       COPYOVER_STATE_FILE=str(runtime/'copyover/state.dat'),
                       DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(websocket))
            output_path = runtime/'server.out'
            with output_path.open('w') as output:
                def boot():
                    offset = output_path.stat().st_size
                    proc = subprocess.Popen([str(runtime/'bin/server/dms'), '--minimal', '-s', str(plain)],
                                            cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
                    deadline = time.monotonic()+120
                    while b'Entering game loop.' not in output_path.read_bytes()[offset:]:
                        assert proc.poll() is None and time.monotonic() < deadline, 'ward fixture boot failed'
                        time.sleep(.1)
                    return proc

                def stop():
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    assert process.returncode == 0

                def save_and_check(previous_capacity):
                    client.send('score')
                    client.expect('Ward status:')
                    client.expect('Minor Globe')
                    client.send('save')
                    client.expect('Save complete for '+journey.CHARACTER+'.', timeout=30)
                    rows = sql(f'SELECT type,duration,ward_source_uid,ward_full_duration,ward_capacity,'
                               f'ward_capacity_max,ward_refresh_remaining,ward_source_type,ward_source_worn,'
                               f'ward_active FROM player_affects WHERE pid={pid} AND ward_source_type>0 ORDER BY type')
                    values = [[int(value) for value in row.split('\t')] for row in rows.splitlines()]
                    assert len(values) == 2, rows
                    cast, equipment = values
                    assert cast[0] == 61 and cast[2:4] == [0, 6000] and cast[5:] == [900000000, 0, 1, 1, 1], rows
                    assert 0 < cast[4] <= previous_capacity and 0 < cast[1] <= 3000, rows
                    assert equipment == [144, 2400, 99112342, 2400, 0, 3200000000, 123, 2, 0, 0], rows
                    return cast[4]

                try:
                    process = boot()
                    client = journey.MudClient(plain)
                    journey.create_character(client)
                    client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.', timeout=30)
                    client.send('quit'); client.expect('ACCOUNT MENU', timeout=30)
                    client.close(); client = None
                    stop()
                    pid = int(sql("SELECT pid FROM player_data WHERE name='"+journey.CHARACTER+"'"))
                    # Seed only this synthetic player's rows while the disposable server is stopped.
                    # 16384 is AFFTYPE_SPELL_WARD, 1 SHORT, 256 NOAPPLY, 2 NOSHOW, 4 NODISPEL.
                    sql(f'INSERT INTO player_affects (pid,type,duration,flags,bitvector1,bitvector2,'
                        f'ward_source_uid,ward_full_duration,ward_capacity,ward_capacity_max,'
                        f'ward_refresh_remaining,ward_source_type,ward_source_worn,ward_active) VALUES '
                        f'({pid},61,3000,16385,64,0,0,6000,450000000,900000000,0,1,1,1),'
                        f'({pid},144,2400,16646,0,2048,99112342,2400,0,3200000000,123,2,0,0)')
                    process = boot(); client = journey.reconnect_character(plain)
                    capacity = save_and_check(450000000)
                    process.send_signal(signal.SIGUSR1)
                    client.expect('Copyover complete!', timeout=90)
                    capacity = save_and_check(capacity)
                    assert output_path.read_text().count('Entering game loop.') == 3
                    client.send('quit'); client.expect('ACCOUNT MENU', timeout=30)
                    client.close(); client = None; stop()
                    process = boot(); client = journey.reconnect_character(plain)
                    save_and_check(capacity)
                    print('weakened capacity, pulse lifetime, broken source UID and paused renewal survived SQL save, restart and exec copyover', flush=True)
                except Exception:
                    print(output_path.read_text(errors='replace')[-6000:])
                    print(journey.runtime_logs(runtime))
                    if client:
                        print(client.transcript.decode(errors='replace')[-5000:])
                    raise
                finally:
                    if client:
                        client.close(); client = None
                    if process and process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=30)
                        except subprocess.TimeoutExpired:
                            process.kill(); process.wait(timeout=10)
    finally:
        sql('DROP DATABASE '+database, False)


if __name__ == '__main__':
    run(Path(sys.argv[1]).resolve())
