#!/usr/bin/env python3
"""Real account/character combat and death on an isolated MariaDB schema.

Set TEST_DB_HOST (loopback), TEST_DB_USER and TEST_DB_PASSWORD for a disposable
server; TEST_DB_PORT defaults to 3306. No checkout .env or existing schema is
used. --server selects a MariaDB executable; by default this script builds
bin/server/dms_new.

TEST_DB_DISPOSABLE=1 is mandatory. The default dispute case proves recovery
AFTER a fixture-side repair. --require-unassisted-recovery keeps all injected
source rows intact, requires a durable retained-conflict death acknowledgement,
then verifies account-menu recovery reads, cold-load refusal, and restart
stability. Use --evidence-dir to choose where transcripts, ordered events, logs,
and exact read-backs are retained; unassisted mode retains them by default.
The guarded retained-conflict owner requires a TEST_MUD development executable;
the disposable environment does not enable it in a production executable.
"""
from pathlib import Path
from collections import Counter
from datetime import datetime, timezone
import argparse
import hashlib
import json
import os
import re
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
    if require_unassisted_recovery:
        environment['TEST_DB_DISPOSABLE'] = '1'
        environment['DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY'] = '1'
    if 'LD_LIBRARY_PATH' in os.environ:
        environment['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH']
    mysql = ['mysql', '--protocol=tcp', '-h', host, '-P', port,
             '-u', environment['DB_USER'], '-N', '-B']

    def sql(text, selected=True):
        return subprocess.check_output(mysql+([database] if selected else []), input=text,
                                       text=True, env=environment).strip()

    def number(text):
        return int(sql(text))

    source_tables = (
        'player_items', 'player_item_affects', 'player_item_extra_descr',
        'player_pets', 'player_pet_items', 'player_pet_item_affects',
        'player_pet_item_extra_descr', 'item_current_owner', 'item_owner_revision',
        'item_ownership_ledger', 'item_ownership_quarantine',
    )
    exact_tables = set(source_tables) | {
        'player_death_conflict_evidence', 'player_death_disposition',
        'player_death_custody',
    }

    def exact_rows(table, where='1=1'):
        if table not in exact_tables:
            raise AssertionError('unapproved exact-readback table: '+table)
        columns = sql(
            "SELECT column_name FROM information_schema.columns "
            "WHERE table_schema=DATABASE() AND table_name='"+table+"' "
            "ORDER BY ordinal_position").splitlines()
        assert columns, 'table unavailable for exact readback: '+table
        primary_key = sql(
            "SELECT column_name FROM information_schema.key_column_usage "
            "WHERE table_schema=DATABASE() AND table_name='"+table+"' "
            "AND constraint_name='PRIMARY' ORDER BY ordinal_position").splitlines()
        order = primary_key or columns
        encoded = [
            "IF(`"+column+"` IS NULL,'N',CONCAT('V',HEX(CAST(`"+column+"` AS BINARY))))"
            for column in columns
        ]
        rows = sql(
            'SELECT CONCAT_WS(\'|\','+','.join(encoded)+') FROM `'+table+'` '
            'WHERE '+where+' ORDER BY '+','.join('`'+column+'`' for column in order))
        return rows.splitlines() if rows else []

    def retained_state(pid):
        return {
            'player': sql(
                f'SELECT save_revision,wallet_revision,copper,silver,gold,platinum,'
                f'numb_deaths,exp,level FROM player_data WHERE pid={pid}'),
            'source_rows': {table: exact_rows(table) for table in source_tables},
            'conflict_cases': exact_rows('player_death_conflict_evidence', f'pid={pid}'),
            'death_dispositions': exact_rows('player_death_disposition', f'pid={pid}'),
            'death_custody': exact_rows('player_death_custody', f'pid={pid}'),
        }

    def readback_digest(state):
        canonical = json.dumps(state, sort_keys=True, separators=(',', ':')).encode()
        return hashlib.sha256(canonical).hexdigest()

    def wallet_state(player_id):
        fields = sql(
            f'SELECT wallet_revision,copper,silver,gold,platinum '
            f'FROM player_data WHERE pid={player_id}').split(chr(9))
        assert len(fields) == 5
        return {
            'wallet_revision': int(fields[0]),
            'cash': [int(value) for value in fields[1:]],
        }

    def owner_revision_value(owner_type, owner_id):
        rows = sql(
            f'SELECT revision FROM item_owner_revision WHERE owner_type={owner_type} '
            f'AND owner_id={owner_id} AND owner_context_id=0').splitlines()
        assert len(rows) <= 1
        return int(rows[0]) if rows else None

    def wallet_coin_rows(player_id):
        projection = sql(
            f'SELECT obj_uid,vnum,item_type,value0,value1,value2,value3,value4,value5,'
            f'value6,value7,quantity FROM player_items WHERE pid={player_id} '
            'ORDER BY obj_uid').splitlines()
        return [line.split(chr(9)) for line in projection if line]

    def appended_rows(before, after, table):
        added = list((Counter(after[table])-Counter(before[table])).elements())
        removed = list((Counter(before[table])-Counter(after[table])).elements())
        assert not removed, f'{table} removed or mutated retained source rows'
        return added

    def verify_wallet_conversion(before_rows, after_rows, player_id,
                                 wallet_before, revisions_before,
                                 owner_revisions_unaffected_before, coin_rows_before):
        # Death first converts the wallet through the ordinary coin-creation
        # transaction. Permit only that one append; all original source rows
        # (including disputed payload/custody) must remain byte-identical.
        coin_rows_after = wallet_coin_rows(player_id)
        added_projection = list(
            (Counter(tuple(row) for row in coin_rows_after)-
             Counter(tuple(row) for row in coin_rows_before)).elements())
        removed_projection = list(
            (Counter(tuple(row) for row in coin_rows_before)-
             Counter(tuple(row) for row in coin_rows_after)).elements())
        assert not removed_projection, 'wallet conversion changed a pre-existing player item'
        assert len(added_projection) == 1, 'wallet conversion did not add exactly one coin pile'
        coin = list(added_projection[0])
        coin_uid = int(coin[0])
        assert coin_uid > 0
        assert coin[1:3] == ['3', '20'], 'new item is not the original money pile'
        assert [int(value) for value in coin[3:7]] == wallet_before['cash'], \
            'coin pile values do not match the debited wallet'
        assert [int(value) for value in coin[7:11]] == [0, 0, 0, 0]
        assert int(coin[11]) == 1

        for table in source_tables:
            if table in ('player_items', 'item_current_owner',
                         'item_owner_revision', 'item_ownership_ledger'):
                continue
            assert after_rows[table] == before_rows[table], \
                f'{table} changed during retained-death wallet conversion'
        added_player_items = appended_rows(before_rows, after_rows, 'player_items')
        added_custody_rows = appended_rows(before_rows, after_rows, 'item_current_owner')
        added_ledger_rows = appended_rows(before_rows, after_rows, 'item_ownership_ledger')
        assert len(added_player_items) == 1, \
            'wallet conversion did not append exactly one original item row'
        assert len(added_custody_rows) == 1, \
            'wallet conversion did not append exactly one custody row'
        assert len(added_ledger_rows) == 1, \
            'wallet conversion did not append exactly one ownership-ledger row'
        assert exact_rows('player_items', f'pid={player_id} AND obj_uid={coin_uid}') == \
            added_player_items, 'durable coin item is not the exact newly appended item row'
        assert exact_rows('item_current_owner', f'item_uid={coin_uid}') == added_custody_rows, \
            'durable coin custody is not the exact newly appended ownership row'
        assert exact_rows('item_ownership_ledger', f'item_uid={coin_uid}') == added_ledger_rows, \
            'durable coin ledger is not the exact newly appended transfer row'

        owner_revision_where = (
            f'NOT ((owner_type=7 AND owner_id=0 AND owner_context_id=0) OR '
            f'(owner_type=1 AND owner_id={player_id} AND owner_context_id=0))')
        assert exact_rows('item_owner_revision', owner_revision_where) == \
            owner_revisions_unaffected_before, \
            ('unrelated item-owner revision rows changed during wallet conversion: ' +
             repr({'before': owner_revisions_unaffected_before,
                   'after': exact_rows('item_owner_revision', owner_revision_where)}))
        revision_after = {
            'system': number(
                'SELECT revision FROM item_owner_revision WHERE owner_type=7 '
                'AND owner_id=0 AND owner_context_id=0'),
            'player': number(
                f'SELECT revision FROM item_owner_revision WHERE owner_type=1 '
                f'AND owner_id={player_id} AND owner_context_id=0'),
        }
        for key in ('system', 'player'):
            prior = revisions_before[key]
            assert revision_after[key] == (0 if prior is None else prior)+1, \
                f'{key} custody revision did not advance exactly once for the coin'

        wallet_after = wallet_state(player_id)
        assert wallet_after['wallet_revision'] == wallet_before['wallet_revision']+1, \
            'wallet revision did not advance exactly once for conversion'
        assert wallet_after['cash'] == [0, 0, 0, 0], \
            'terminal death retained spendable wallet funds'

        custody = sql(
            f"SELECT item_uid,root_item_uid,IFNULL(CAST(parent_item_uid AS CHAR),'NULL'),"
            f"owner_type,owner_id,owner_context_id,item_revision,vnum,state,"
            f"IFNULL(HEX(coin_payload),'NULL') FROM item_current_owner "
            f'WHERE item_uid={coin_uid}').split(chr(9))
        assert len(custody) == 10 and custody[:9] == [
            str(coin_uid), str(coin_uid), 'NULL', '1', str(player_id), '0', '1', '3', '1'
        ], 'new coin custody row does not match its durable player payload'
        assert custody[9] not in ('', 'NULL'), 'coin custody row omitted authoritative payload'

        ledger = sql(
            f"SELECT item_uid,root_item_uid,IFNULL(CAST(parent_item_uid AS CHAR),'NULL'),"
            f'from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,'
            f'to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,'
            f'reason_type,reason_id,source_site,HEX(operation_id),event_index '
            f'FROM item_ownership_ledger WHERE item_uid={coin_uid}').splitlines()
        assert len(ledger) == 1, 'coin conversion did not commit one original custody ledger entry'
        ledger_fields = ledger[0].split(chr(9))
        assert ledger_fields[:13] == [
            str(coin_uid), str(coin_uid), 'NULL', '7', '0', '0', '1', str(player_id),
            '0', '1', str(revision_after['system']), str(revision_after['player']), '2'
        ], 'coin ledger does not prove the exact system-to-player creation transfer'
        assert len(ledger_fields[15]) == 32 and ledger_fields[16] == '0'

        return {
            'wallet_before': wallet_before,
            'wallet_after': wallet_after,
            'wallet_debit': wallet_before['cash'],
            'coin_projection': coin,
            'coin_custody': custody,
            'coin_ledger': ledger_fields,
            'owner_revisions_before': revisions_before,
            'owner_revisions_after': revision_after,
            'allowed_source_row_additions': {
                'player_items': added_player_items,
                'item_current_owner': added_custody_rows,
                'item_ownership_ledger': added_ledger_rows,
            },
        }

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
        'ordered_events': [],
        'transcripts': {},
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
    if evidence_dir is not None or require_unassisted_recovery:
        evidence_root = (Path(evidence_dir).resolve() if evidence_dir is not None else
                         ROOT/'bin/tests/mysql-death-conflict-evidence')
        evidence_path = evidence_root / database
        evidence_path.mkdir(parents=True, mode=0o700, exist_ok=False)
        (evidence_path/'transcripts').mkdir(mode=0o700)
        for name in ('ordered-events.jsonl', 'ordered-events.log'):
            path = evidence_path/name
            path.touch(mode=0o600)
            path.chmod(0o600)

    def record_event(stage, **details):
        event = {
            'sequence': len(evidence['ordered_events'])+1,
            'stage': stage,
            'observed_utc': datetime.now(timezone.utc).isoformat(),
            'details': details,
        }
        evidence['ordered_events'].append(event)
        if evidence_path is not None:
            with (evidence_path/'ordered-events.jsonl').open('a') as stream:
                stream.write(json.dumps(event, sort_keys=True)+chr(10))
            with (evidence_path/'ordered-events.log').open('a') as stream:
                stream.write(f"{event['sequence']:03d} {stage} {json.dumps(details, sort_keys=True)}"+chr(10))

    def save_transcript(label, mud_client):
        if evidence_path is None or mud_client is None:
            return
        path = evidence_path/'transcripts'/(label+'.server-output.txt')
        content = bytes(mud_client.transcript)
        path.write_bytes(content)
        path.chmod(0o600)
        evidence['transcripts'][label] = {
            'path': str(path.relative_to(evidence_path)),
            'server_output_bytes': len(content),
            'server_output_sha256': hashlib.sha256(content).hexdigest(),
        }

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

                def authenticate_account(port):
                    account_client = journey.MudClient(port)
                    try:
                        entry, _ = account_client.expect_any(('term type', 'account name'))
                        if entry == 'term type':
                            account_client.send('9')
                            account_client.expect('account name')
                        account_client.send(journey.ACCOUNT)
                        account_client.expect('enter your password')
                        account_client.send(journey.PASSWORD)
                        account_client.expect('PRESS RETURN')
                        account_client.send('')
                        menu = account_client.expect('Please select an option')
                        assert 'ACCOUNT MENU' in menu
                        assert '9) Review death-recovery records' in menu
                        return account_client
                    except Exception:
                        account_client.close()
                        raise

                def review_recovery(account_client, case_id, save_revision,
                                    source_revision, transcript_label):
                    account_client.send('9')
                    account_client.expect('Which character would you like to play?')
                    account_client.send('1')
                    listing = account_client.expect('Recovery choice:', timeout=30)
                    cases = re.findall(
                        r'Case ([0-9a-fA-F]{32}) \| save revision (\d+) \| source revision (\d+)',
                        listing)
                    assert len(cases) == 1, 'recovery list did not show exactly one retained case'
                    shown_id, shown_revision, shown_source = cases[0]
                    assert shown_id.lower() == case_id.lower()
                    assert int(shown_revision) == save_revision
                    assert int(shown_source) == source_revision
                    assert 'Status: unresolved archive evidence only' in listing
                    record_event('recovery_list_read', case_id=case_id,
                                 save_revision=save_revision, source_revision=source_revision)

                    account_client.send('D '+case_id)
                    detail = account_client.expect('Recovery choice:', timeout=30)
                    assert ('Detail for retained case '+case_id+':').lower() in detail.lower()
                    assert f'Saved revision {save_revision}; source revision {source_revision}.' in detail
                    assert 'Evidence rows (historical, not inventory)' in detail
                    assert 'Raw payload withheld.' in detail
                    record_event('recovery_detail_read', case_id=case_id,
                                 save_revision=save_revision, source_revision=source_revision)
                    save_transcript(transcript_label, account_client)

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
                    source_rows_before_terminal = {}
                    wallet_before_terminal = None
                    owner_revisions_before = None
                    owner_revisions_unaffected_before = None
                    coin_rows_before = None
                    if require_unassisted_recovery:
                        wallet_before_terminal = wallet_state(pid)
                        assert any(wallet_before_terminal['cash']), \
                            'retained-death fixture must have a wallet to convert'
                        owner_revisions_before = {
                            'system': owner_revision_value(7, 0),
                            'player': owner_revision_value(1, pid),
                        }
                        owner_revision_where = (
                            f'NOT ((owner_type=7 AND owner_id=0 AND owner_context_id=0) OR '
                            f'(owner_type=1 AND owner_id={pid} AND owner_context_id=0))')
                        owner_revisions_unaffected_before = exact_rows(
                            'item_owner_revision', owner_revision_where)
                        coin_rows_before = wallet_coin_rows(pid)
                        source_rows_before_terminal = {
                            table: exact_rows(table) for table in source_tables
                        }
                        evidence['wallet_before_terminal'] = wallet_before_terminal
                        evidence['owner_revisions_before_terminal'] = owner_revisions_before
                        evidence['owner_revisions_unaffected_before_terminal'] = \
                            owner_revisions_unaffected_before
                        evidence['source_rows_before_terminal'] = source_rows_before_terminal
                        record_event(
                            'conflict_fixture_injected', pid=pid, stray_uid=stray,
                            source_rows_sha256=readback_digest(source_rows_before_terminal))
                        save_transcript('conflict-death-and-account-recovery', client)
                    began = time.monotonic()
                    journey.attack_until_death(client)
                    if require_unassisted_recovery:
                        record_event('actual_player_death_observed', pid=pid)
                        deadline = time.monotonic()+45
                        while True:
                            logs = (runtime/'logs/log/file').read_bytes()[conflict_log_offset:].decode(errors='replace')
                            case_count = number(
                                f'SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid={pid}')
                            if case_count:
                                case_headers = sql(
                                    f'SELECT HEX(operation_id),save_revision,source_revision,corpse_item_uid '
                                    f'FROM player_death_conflict_evidence WHERE pid={pid} ORDER BY save_revision')
                            else:
                                case_headers = ''
                            if ('death_disposition_recorded' in logs and
                                'death_disposition_completed' in logs and case_count == 1 and
                                len(case_headers.splitlines()) == 1):
                                case_id, save_revision_text, source_revision_text, corpse_uid_text = \
                                    case_headers.splitlines()[0].split(chr(9))
                                save_revision = int(save_revision_text)
                                source_revision = int(source_revision_text)
                                terminal_receipt_count = number(
                                    f"SELECT COUNT(*) FROM player_death_disposition WHERE pid={pid} "
                                    f"AND save_revision={save_revision} "
                                    f"AND operation_id=UNHEX('{case_id}')")
                                saved_revision = number(
                                    f'SELECT save_revision FROM player_data WHERE pid={pid}')
                                if terminal_receipt_count == 1 and saved_revision == save_revision:
                                    break
                            if time.monotonic() >= deadline:
                                raise AssertionError(
                                    'retained death case and terminal ACK did not become durable')
                            # The menu must not even be waiting in the socket before the
                            # durable receipt, terminal revision, and completion marker.
                            previous_timeout = client.socket.gettimeout()
                            client.socket.settimeout(0.01)
                            try:
                                client._receive()
                            finally:
                                client.socket.settimeout(previous_timeout)
                            assert b'ACCOUNT MENU' not in client.pending, \
                                'ACCOUNT MENU arrived before durable retained-death ACK'
                            time.sleep(.05)

                        assert save_revision > source_revision
                        assert corpse_uid_text.isdigit()
                        assert logs.index('death_disposition_recorded') < logs.index(
                            'death_disposition_completed')
                        record_event('durable_death_ack_observed', case_id=case_id,
                                     save_revision=save_revision,
                                     source_revision=source_revision)
                        record_state('retained_conflict_durable_ack')

                        state_at_ack = retained_state(pid)
                        evidence['retained_state_after_ack'] = state_at_ack
                        assert len(state_at_ack['conflict_cases']) == 1
                        assert len(state_at_ack['death_dispositions']) == 1
                        wallet_conversion = verify_wallet_conversion(
                            source_rows_before_terminal, state_at_ack['source_rows'], pid,
                            wallet_before_terminal, owner_revisions_before,
                            owner_revisions_unaffected_before, coin_rows_before)
                        evidence['wallet_conversion_at_ack'] = wallet_conversion
                        record_event(
                            'wallet_conversion_accounted_at_durable_ack',
                            coin_uid=wallet_conversion['coin_projection'][0],
                            wallet_revision_delta=1,
                            source_rows_sha256=readback_digest(
                                wallet_conversion['allowed_source_row_additions']))
                        evidence['retained_state_after_ack'] = state_at_ack
                        player_fields = state_at_ack['player'].split(chr(9))
                        assert int(player_fields[0]) == save_revision
                        assert player_fields[2:6] == ['0', '0', '0', '0'], \
                            'terminal death retained a spendable wallet balance'
                        assert int(player_fields[6]) == before_deaths+1, \
                            'death counter was not committed exactly once'
                        assert number(
                            f"SELECT COUNT(*) FROM player_death_custody WHERE pid={pid} "
                            f"AND save_revision={save_revision}") > 0
                        evidence['retained_state_after_ack'] = state_at_ack
                        ack_digest = readback_digest(state_at_ack)
                        evidence['retained_state_after_ack_sha256'] = ack_digest
                        evidence['case_identity'] = {
                            'operation_id': case_id,
                            'save_revision': save_revision,
                            'source_revision': source_revision,
                            'corpse_item_uid': int(corpse_uid_text),
                        }

                        client.expect('ACCOUNT MENU', timeout=45)
                        menu = client.expect('Please select an option', timeout=15)
                        assert '9) Review death-recovery records' in menu
                        evidence['account_menu_reached'] = True
                        evidence['conflict_to_menu_seconds'] = time.monotonic()-began
                        record_event('account_menu_after_durable_ack', case_id=case_id,
                                     save_revision=save_revision)
                        review_recovery(client, case_id, save_revision, source_revision,
                                        'conflict-death-and-account-recovery')

                        client.send('0')
                        client.expect('Please select an option')
                        client.send('1')
                        client.expect('Which character would you like to play?')
                        client.send('1')
                        client.expect('Play as '+journey.CHARACTER+'? (Y/N)')
                        client.send('y')
                        refusal = client.expect(
                            'This character has retained death-recovery evidence. The character was not loaded; '
                            'review account-menu option 9.', timeout=30)
                        assert 'not loaded' in refusal
                        returned_menu = client.expect('Please select an option', timeout=30)
                        assert 'ACCOUNT MENU' in returned_menu
                        record_event('self_scoped_cold_entry_refused', case_id=case_id,
                                     save_revision=save_revision)
                        state_after_refusal = retained_state(pid)
                        assert state_after_refusal == state_at_ack, \
                            'cold-entry refusal changed retained evidence or source state'
                        evidence['retained_state_after_cold_refusal'] = state_after_refusal
                        evidence['cold_entry_refusal_readback_sha256'] = readback_digest(
                            state_after_refusal)
                        save_transcript('conflict-death-and-account-recovery', client)

                        client.send('0')
                        client.expect('Thank you for playing!', timeout=15)
                        client.close()
                        client = None
                        stop()
                        record_event('server_stopped_before_restart')
                        process = boot()
                        record_event('game_server_restarted')

                        client = authenticate_account(plain)
                        record_event('authenticated_account_reconnected')
                        review_recovery(client, case_id, save_revision, source_revision,
                                        'post-restart-account-recovery')
                        state_after_restart = retained_state(pid)
                        assert state_after_restart == state_at_ack, \
                            'restart/reconnect changed retained case, source rows, wallet, or death count'
                        evidence['retained_state_after_restart'] = state_after_restart
                        evidence['restart_readback_sha256'] = readback_digest(state_after_restart)
                        evidence['restart_readback_equal_to_ack'] = True
                        record_event('retained_state_stable_after_restart', case_id=case_id,
                                     readback_sha256=evidence['restart_readback_sha256'])
                        client.send('0')
                        client.expect('Please select an option')
                        client.send('0')
                        client.expect('Thank you for playing!', timeout=15)
                        save_transcript('post-restart-account-recovery', client)
                        client.close()
                        client = None
                        stop()
                        assert (runtime/'Players/pc_idnumb').is_file(), \
                            'SQL-mode player ID file was not created'
                        fixture_logs = journey.runtime_logs(runtime)
                        assert 'could not open pc_idnumb file for writing' not in fixture_logs
                        assert 'sql_save_player_shapechanges failed' not in fixture_logs
                        record_state('restart_stable')
                        evidence['result'] = 'passed'
                        record_event('retained_conflict_journey_passed', case_id=case_id)
                        print(
                            'MariaDB retained conflict: durable ACK before account menu; '
                            'self-scoped list/detail; cold entry refused; restart stable; '
                            f'manual_fixture_repair={evidence["manual_fixture_repair"]}',
                            flush=True)
                        return
                    deadline=time.monotonic()+15
                    while 'custody_payload_mismatch_rejected' not in (runtime/'logs/log/file').read_bytes()[conflict_log_offset:].decode(errors='replace'):
                        assert time.monotonic()<deadline, 'uncaptured payload was not rejected'
                        time.sleep(.05)
                    record_state('after_conflict_death')
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
                    if client:
                        save_transcript('failure-current-session', client)
                        client.close()
                    if process and process.poll() is None:
                        process.terminate()
                        try: process.wait(timeout=10)
                        except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=10)
                    if evidence_path is not None:
                        evidence['finished_utc'] = datetime.now(timezone.utc).isoformat()
                        shutil.copytree(runtime/'logs', evidence_path/'logs')
                        shutil.copytree(runtime/'journals', evidence_path/'journals')
                        shutil.copy2(output_path, evidence_path/'server.out')
                        (evidence_path/'server.out').chmod(0o600)
                        (evidence_path/'custody-evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
                        (evidence_path/'custody-evidence.json').chmod(0o600)
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
        print('SKIP: MariaDB live combat requires TEST_DB_HOST for a disposable fixture')
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
