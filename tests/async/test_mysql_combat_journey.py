#!/usr/bin/env python3
"""Real account/character combat and death on an isolated MariaDB schema.

Set TEST_DB_HOST (loopback), TEST_DB_USER and TEST_DB_PASSWORD for a disposable
server; TEST_DB_PORT defaults to 3306. No checkout .env or existing schema is
used. --server selects a freshly built MariaDB executable; by default this
script builds bin/server/dms_new.

TEST_DB_DISPOSABLE=1 is mandatory. The default dispute case proves recovery
AFTER a fixture-side repair. --require-unassisted-recovery leaves that conflict
in place and requires the server to complete death without deleting evidence.
Use --evidence-dir to retain full logs and before/after custody observations on
both success and failure. A passing default case does not qualify unassisted
conflict recovery or player-visible item restitution.
"""
from pathlib import Path
from datetime import datetime, timezone
import argparse
import hashlib
import json
import os
import shutil
import signal
import subprocess
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def run(server, reset_coins=False, boons=False, *, require_unassisted_recovery=False,
        evidence_dir=None):
    if os.environ.get('TEST_DB_DISPOSABLE') != '1':
        raise RuntimeError('TEST_DB_DISPOSABLE=1 is required')
    database = 'corpse_journey_test_' + uuid.uuid4().hex[:12]
    host = os.environ['TEST_DB_HOST']
    port = os.environ.get('TEST_DB_PORT', '3306')
    assert host in ('127.0.0.1', 'localhost'), 'use a disposable loopback database'
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

    evidence = {
        'database': database,
        'server': str(server),
        'source_head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        'started_utc': datetime.now(timezone.utc).isoformat(),
        'require_unassisted_recovery': require_unassisted_recovery,
        'reset_coins': reset_coins,
        'boons': boons,
        'manual_fixture_repair': False,
        'result': 'not_completed',
        'observations': [],
    }
    for label, path in (('server_sha256', server), ('test_sha256', __file__)):
        digest = hashlib.sha256()
        with Path(path).open('rb') as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b''):
                digest.update(chunk)
        evidence[label] = digest.hexdigest()
    tracked_uids = set()
    pid = None
    evidence_path = None
    if evidence_dir is not None:
        evidence_path = Path(evidence_dir).resolve() / database
        evidence_path.mkdir(parents=True, mode=0o700, exist_ok=False)

    def record_state(stage):
        if pid is None:
            return
        uids = ','.join(str(uid) for uid in sorted(tracked_uids)) or '0'
        evidence['observations'].append({
            'stage': stage,
            'pid': pid,
            'observed_utc': datetime.now(timezone.utc).isoformat(),
            'canonical_log_bytes': (runtime/'logs/log/file').stat().st_size,
            'player': sql(f'SELECT save_revision,wallet_revision,copper,silver,gold,platinum,numb_deaths FROM player_data WHERE pid={pid}'),
            'inventory': sql(f'SELECT obj_uid,vnum,equip_slot,container_id,quantity FROM player_items WHERE pid={pid} ORDER BY obj_uid'),
            'inventory_full_rows': sql(f'SELECT * FROM player_items WHERE pid={pid} ORDER BY obj_uid'),
            'corpses_full_rows': sql(f'SELECT * FROM corpses WHERE player_name=(SELECT name FROM player_data WHERE pid={pid}) ORDER BY id'),
            'corpse_items_full_rows': sql(f'SELECT ci.* FROM corpse_items ci JOIN corpses c ON c.id=ci.corpse_id WHERE c.player_name=(SELECT name FROM player_data WHERE pid={pid}) ORDER BY ci.corpse_id'),
            'custody': sql(f'SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid IN ({uids}) OR (owner_type=1 AND owner_id={pid}) ORDER BY item_uid'),
            'death_receipts': sql(f'SELECT save_revision,HEX(operation_id),SHA2(payload,256) FROM player_death_disposition WHERE pid={pid} ORDER BY save_revision'),
            'death_custody': sql(f'SELECT save_revision,item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,item_revision,state FROM player_death_custody WHERE pid={pid} ORDER BY save_revision,item_uid'),
        })

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
            # SQL-mode player IDs still use SAVE_DIR/pc_idnumb during creation.
            (runtime/'Players').mkdir(mode=0o700)
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
                    tracked_uids.update(int(uid) for uid in captured)
                    assert len(captured)>2, 'fixture did not retain a multi-root inventory'
                    record_state('before_healthy_death')
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
                    client.send('quit'); client.expect('ACCOUNT MENU',timeout=30)
                    client.send('0'); client.close(); client=None
                    # Add a durable child absent from the live object graph.
                    # A cold load must retain the valid graph read-only and route
                    # death through its immutable disposition.
                    ghost=9000000000000000000+pid
                    tracked_uids.update((banana, ghost, ghost+1))
                    sql(f'INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,item_revision,vnum,state) VALUES({ghost},{banana},{banana},1,{pid},1,15,1)')
                    client=journey.reconnect_character(plain)
                    client.send('inventory'); client.expect('a banana',timeout=15)
                    deadline=time.monotonic()+15
                    while 'outcome=missing_payload_rows' not in journey.runtime_logs(runtime):
                        assert time.monotonic()<deadline, 'payload gap was not reported at load'
                        time.sleep(.01)
                    assert number(f'SELECT COUNT(*) FROM item_current_owner WHERE item_uid={ghost} AND owner_type=1 AND owner_id={pid} AND state=1')==1
                    before_deaths=number(f'SELECT numb_deaths FROM player_data WHERE pid={pid}')
                    # A newly discovered payload outside the captured corpse
                    # must not be discarded or released on journal append alone.
                    # The legacy guard case repairs this row explicitly. The
                    # release-qualification case MUST recover without that edit.
                    stray=ghost+1
                    sql(f'INSERT INTO player_items (pid,vnum,equip_slot,container_id,quantity,item_type,obj_uid) VALUES ({pid},15,0,NULL,1,0,{stray})')
                    record_state('conflict_injected')
                    evidence['unrepresented_payload_before'] = sql(f'SELECT * FROM player_items WHERE pid={pid} AND obj_uid={stray}')
                    conflict_log_offset = (runtime/'logs/log/file').stat().st_size
                    evidence['conflict_log_offset'] = conflict_log_offset
                    journey.attack_until_death(client)
                    deadline=time.monotonic()+15
                    while 'custody_payload_mismatch_rejected' not in (runtime/'logs/log/file').read_bytes()[conflict_log_offset:].decode(errors='replace'):
                        if require_unassisted_recovery and 'death_disposition_completed' in (runtime/'logs/log/file').read_bytes()[conflict_log_offset:].decode(errors='replace'):
                            break
                        assert time.monotonic()<deadline, 'uncaptured payload was not rejected'
                        time.sleep(.05)
                    record_state('after_conflict_death')
                    if not require_unassisted_recovery:
                        conflict_logs = (runtime/'logs/log/file').read_bytes()[conflict_log_offset:].decode(errors='replace')
                        assert 'death_disposition_completed' not in conflict_logs
                        assert number(f'SELECT COUNT(*) FROM player_items WHERE pid={pid} AND obj_uid={stray}')==1
                        sql(f'DELETE FROM player_items WHERE pid={pid} AND obj_uid={stray}')
                        evidence['manual_fixture_repair'] = True
                    began = time.monotonic()
                    try:
                        client.expect('ACCOUNT MENU',timeout=45)
                    except Exception:
                        evidence['conflict_to_menu_seconds'] = time.monotonic()-began
                        evidence['account_menu_reached'] = False
                        record_state('blocked_before_client_close')
                        raise
                    evidence['conflict_to_menu_seconds'] = time.monotonic()-began
                    evidence['account_menu_reached'] = True
                    record_state('account_menu_reached')
                    client.send('0'); client.close(); client=None
                    # The account menu may follow a durable journal handoff
                    # before the asynchronous MariaDB worker acknowledges it.
                    deadline=time.monotonic()+20
                    while True:
                        logs=(runtime/'logs/log/file').read_bytes()[conflict_log_offset:].decode(errors='replace')
                        disposition_count=number(f'SELECT COUNT(*) FROM player_death_disposition WHERE pid={pid}')
                        if ('load_item_payload_gap_disposition' in logs and
                            'death_disposition_completed' in logs and disposition_count==1):
                            break
                        assert time.monotonic()<deadline, 'death disposition did not reach MariaDB after journal handoff'
                        time.sleep(.05)
                    assert 'load_item_payload_gap_disposition' in logs
                    assert logs.index('death_disposition_recorded')<logs.index('death_disposition_completed')
                    assert number(f'SELECT COUNT(*) FROM player_death_custody WHERE pid={pid} AND item_uid={banana} AND owner_type=1')==1
                    assert number(f'SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND state=1')==0
                    assert number(f'SELECT numb_deaths FROM player_data WHERE pid={pid}')==before_deaths+1
                    if require_unassisted_recovery:
                        # A reviewed durable reconciliation route may supersede
                        # this retention assertion only with exact original
                        # payload/UID read-back; completion must not erase it.
                        assert sql(f'SELECT * FROM player_items WHERE pid={pid} AND obj_uid={stray}')==evidence['unrepresented_payload_before'], 'unrepresented payload was erased or changed instead of reconciled'
                    before=stable_state(pid)
                    stop(); process=boot()
                    client=journey.reconnect_character(plain)
                    client.send('save'); client.expect('Save complete for '+journey.CHARACTER+'.')
                    client.send('quit'); client.expect('ACCOUNT MENU',timeout=30)
                    client.send('0'); client.close(); client=None
                    assert stable_state(pid)==before, 'restart duplicated death consequences or rewrote evidence'
                    stop()
                    assert (runtime/'Players/pc_idnumb').is_file(), 'SQL-mode player ID file was not created'
                    fixture_logs=journey.runtime_logs(runtime)
                    assert 'could not open pc_idnumb file for writing' not in fixture_logs
                    assert 'sql_save_player_shapechanges failed' not in fixture_logs
                    record_state('restart_stable')
                    evidence['result'] = 'passed'
                    print(f'MariaDB disputed death: durable before release; restart stable; manual_fixture_repair={evidence["manual_fixture_repair"]}; reset_coins={reset_coins}, boons={boons}',flush=True)
                except Exception as error:
                    evidence['result'] = 'failed'
                    evidence['failure_type'] = type(error).__name__
                    raise AssertionError(str(error)+'\n'+output_path.read_text(errors='replace')[-10000:]+'\n'+journey.runtime_logs(runtime)) from error
                finally:
                    if client: client.close()
                    if process and process.poll() is None:
                        process.terminate()
                        try: process.wait(timeout=10)
                        except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=10)
                    if evidence_path is not None:
                        evidence['finished_utc'] = datetime.now(timezone.utc).isoformat()
                        shutil.copytree(runtime/'logs', evidence_path/'logs')
                        shutil.copytree(runtime/'journals', evidence_path/'journals')
                        shutil.copy2(output_path, evidence_path/'server.out')
                        (evidence_path/'custody-evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
                        print(f'full_runtime_evidence={evidence_path}', flush=True)
    finally:
        sql('DROP DATABASE '+database,False)


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--server',type=Path)
    parser.add_argument('--one',action='store_true',help='run only the default-coin variant')
    parser.add_argument('--require-unassisted-recovery',action='store_true',help='do not manually remove the injected payload conflict (release gate)')
    parser.add_argument('--evidence-dir',type=Path,help='retain full runtime logs and synthetic custody evidence')
    args=parser.parse_args()
    if not os.getenv('TEST_DB_HOST'):
        print('MariaDB live combat skipped: TEST_DB_HOST is not set')
    else:
        (ROOT/'bin/tests').mkdir(parents=True,exist_ok=True)
        if not args.server:
            subprocess.run(['make','-C','src','-j2','PERSISTENCE_BACKEND=mariadb'],cwd=ROOT,check=True)
        server=(args.server or ROOT/'bin/server/dms_new').resolve()
        options = dict(require_unassisted_recovery=args.require_unassisted_recovery,
                       evidence_dir=args.evidence_dir)
        run(server, **options)
        if not args.one:
            run(server,reset_coins=True, **options)
            run(server,boons=True, **options)
