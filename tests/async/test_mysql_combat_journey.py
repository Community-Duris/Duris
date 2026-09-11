#!/usr/bin/env python3
"""Real account/character combat and death on an isolated MariaDB schema.

Set TEST_DB_HOST (loopback), TEST_DB_USER and TEST_DB_PASSWORD for a disposable
server. No checkout .env or existing schema is used. --server selects a freshly
built MariaDB executable; by default this script builds bin/server/dms_new.
"""
from pathlib import Path
import argparse
import errno
import os
import signal
import subprocess
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def run(server, reset_coins=False, boons=False):
    database = 'corpse_journey_test_' + uuid.uuid4().hex[:12]
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
            journey.make_fixture(runtime, reset_coins)
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

                def stable_state(pid):
                    return (
                        sql(f'SELECT copper,silver,gold,platinum,wallet_revision,numb_deaths,exp,level FROM player_data WHERE pid={pid}'),
                        sql(f'SELECT save_revision,HEX(operation_id),SHA2(payload,256) FROM player_death_disposition WHERE pid={pid} ORDER BY save_revision'),
                        sql(f'SELECT item_uid,owner_type,owner_id,state FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} ORDER BY item_uid'),
                    )

                try:
                    process = boot()
                    client = journey.MudClient(plain)
                    journey.create_character(client)
                    if not boons:
                        client.send('toggle boon'); client.expect('You will no longer be affected by boons.')
                    journey.complete_npc_combat_journey(client, reset_coins)
                    client.close(); client = journey.reconnect_character(plain)
                    # Recover the starter-kit roots dropped by the NPC fixture,
                    # so the real player's death exercises a multi-root batch.
                    client.send('get all'); client.expect('You get', timeout=20)
                    client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.', timeout=30)
                    pid = number("SELECT pid FROM player_data WHERE name='"+journey.CHARACTER+"'")
                    captured = sql(f'SELECT item_uid FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND state=1 ORDER BY item_uid').splitlines()
                    assert len(captured)>2, 'fixture did not retain a multi-root inventory'
                    before_deaths=number(f'SELECT numb_deaths FROM player_data WHERE pid={pid}')
                    began=time.monotonic(); journey.attack_until_death(client)
                    client.expect('ACCOUNT MENU',timeout=45)
                    elapsed=time.monotonic()-began
                    client.send('0'); client.close(); client=None
                    uids=','.join(captured)
                    assert number(f'SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN ({uids}) AND owner_type=4 AND state=1')==len(captured), sql(f'SELECT item_uid,owner_type,owner_id,state FROM item_current_owner WHERE item_uid IN ({uids})')
                    assert number(f'SELECT COUNT(DISTINCT HEX(operation_id)) FROM item_ownership_ledger WHERE item_uid IN ({uids}) AND reason_type=9')==1
                    assert number(f'SELECT numb_deaths FROM player_data WHERE pid={pid}')==before_deaths+1
                    assert number("SELECT COUNT(*) FROM corpse_items ci JOIN corpses c ON c.id=ci.corpse_id WHERE c.player_name='"+journey.CHARACTER+"'")>=len(captured)
                    print(f'MariaDB actual character: {len(captured)} captured items, one corpse-create command, attack-to-menu {elapsed:.3f}s',flush=True)
                    # Minimal boot deliberately skips SQL corpse restoration.
                    # Verify persisted rows and in-game loot before restarting;
                    # the restart below tests death disposition/player authority.
                    # MariaDB's existing loader also uses the normal login text.
                    client=journey.reconnect_character(plain)
                    client.send('look'); client.expect('The corpse of a Human is lying here.')
                    client.send('look in '+journey.CHARACTER); client.expect('a banana')
                    client.send('get banana '+journey.CHARACTER); client.expect('get a banana',timeout=15)
                    client.send('get coins '+journey.CHARACTER); client.expect('You get 3s.' if reset_coins else 'You get 1c.',timeout=15)
                    client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.')
                    client.send('quit'); client.expect('ACCOUNT MENU',timeout=30)
                    client.send('0'); client.close(); client=None
                    journey.verify_recovered_loot(plain)
                    expected='0\t3\t0\t0' if reset_coins else '1\t0\t0\t0'
                    assert sql(f'SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}')==expected
                    client=journey.reconnect_character(plain)
                    client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.')
                    banana=number(f'SELECT item_uid FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND state=1 AND vnum=15 LIMIT 1')
                    # Add a durable child absent from the live object graph.
                    # The real batch repository must refuse the incomplete tree.
                    ghost=9000000000000000000+pid
                    sql(f'INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,item_revision,vnum,state) VALUES({ghost},{banana},{banana},1,{pid},1,15,1)')
                    before_deaths=number(f'SELECT numb_deaths FROM player_data WHERE pid={pid}')
                    journey.attack_until_death(client)
                    client.expect('ACCOUNT MENU',timeout=45)
                    client.send('0'); client.close(); client=None
                    logs=journey.runtime_logs(runtime)
                    assert f'error={errno.EMSGSIZE} disputed=1' in logs
                    assert logs.index('death_disposition_recorded')<logs.index('death_disposition_completed')
                    assert number(f'SELECT COUNT(*) FROM player_death_disposition WHERE pid={pid}')==1
                    assert number(f'SELECT COUNT(*) FROM player_death_custody WHERE pid={pid} AND item_uid={banana} AND owner_type=1')==1
                    assert number(f'SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND state=1')==0
                    assert number(f'SELECT numb_deaths FROM player_data WHERE pid={pid}')==before_deaths+1
                    before=stable_state(pid)
                    stop(); process=boot()
                    client=journey.reconnect_character(plain)
                    client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.')
                    client.send('quit'); client.expect('ACCOUNT MENU',timeout=30)
                    client.send('0'); client.close(); client=None
                    assert stable_state(pid)==before, 'restart duplicated death consequences or rewrote evidence'
                    stop()
                    print(f'MariaDB disputed death: durable before release; restart stable; reset_coins={reset_coins}, boons={boons}',flush=True)
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


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--server',type=Path)
    parser.add_argument('--one',action='store_true',help='run only the default-coin variant')
    args=parser.parse_args()
    if not os.getenv('TEST_DB_HOST'):
        print('MariaDB live combat skipped: TEST_DB_HOST is not set')
    else:
        (ROOT/'bin/tests').mkdir(parents=True,exist_ok=True)
        if not args.server:
            subprocess.run(['make','-C','src','-j2','PERSISTENCE_BACKEND=mariadb'],cwd=ROOT,check=True)
        server=(args.server or ROOT/'bin/server/dms_new').resolve()
        run(server)
        if not args.one:
            run(server,reset_coins=True)
            run(server,boons=True)
