#!/usr/bin/env python3
"""Account-only lockers, private titles, dedicated saves and shutdown/restart on disposable MariaDB.

Requires a generated full world, a matching server binary and explicit disposable
loopback TEST_DB_HOST/USER/PASSWORD. No checkout .env, production database,
running server or existing player is used.
"""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey
from test_mysql_epic_training_journey import authority, command, import_help
from run_world_quest_dual_backend import setup_run_root

ROOT = journey.ROOT
OWNER, VISITOR, OTHER, GOD = 'Vaultowner', 'Guestchar', 'Otherguest', 'Lockwarden'
ACCOUNTS = {OWNER: 'Ownervault', VISITOR: 'Guestacct', OTHER: 'Otheracct', GOD: 'Staffacct'}
ACCOUNT_LOCKER = 'account.ownervault.1.locker'
ALT, EVIL_ALT = 'Altowner', 'Darkowner'
ACTIVE = tuple(ACCOUNTS)
ACCOUNTS.update({ALT: 'Altfixture', EVIL_ALT: 'Darkfixture'})
BANK = 16590
NEUTRAL_COUNTER = 96421  # Ixarkon welcomes both sides, so the locker enforces racewar.
DENIED = "You don't have access to that locker!"


def run(binary):
    (ROOT / 'bin/tests').mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='locker-grants-', dir=ROOT / 'bin/tests') as temporary:
        runtime = Path(temporary)
        players, critical, output_path = setup_run_root(runtime)
        (runtime / 'help').symlink_to(ROOT / 'help', target_is_directory=True)
        executable = runtime / 'bin/server/dms'
        executable.parent.mkdir(parents=True)
        shutil.copy2(binary, executable)
        clients = {}
        process = None
        evidence = {'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(), 'checks': []}
        plain, tls, websocket = journey.available_ports()
        with authority(runtime) as (env, sql), output_path.open('w') as output:
            env.update(PLAYER_SAVE_JOURNAL_DIR=str(players), CRITICAL_COMMAND_JOURNAL_DIR=str(critical), DURIS_TLS_PORT=str(tls),
                       DURIS_WEBSOCKET_PORT=str(websocket),
                       MAINTENANCE_STATE_FILE=str(runtime / 'maintenance.state'))

            def boot():
                nonlocal process
                offset = output_path.stat().st_size
                process = subprocess.Popen([str(executable), '-d', str(runtime), str(plain)],
                                           cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
                deadline = time.monotonic() + 180
                while b'Entering game loop.' not in output_path.read_bytes()[offset:]:
                    assert process.poll() is None and time.monotonic() < deadline, 'locker world failed to boot'
                    time.sleep(.1)

            def stop(checkpoints=True):
                nonlocal process
                # This is a clean restart journey, not shutdown during an
                # in-flight locker save. Wait for player checkpoints while the
                # game thread continues processing locker completions.
                if checkpoints:
                    for name in tuple(clients):
                        act(name, 'save', 'Save complete for ' + name + '.')
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=60)
                assert process.returncode == 0, 'unclean locker journey shutdown'
                for client in clients.values():
                    client.close()
                clients.clear()
                process = None

            def login(name):
                client = journey.MudClient(plain)
                clients[name] = client
                entry, _ = client.expect_any(('term type', 'account name'))
                if entry == 'term type':
                    client.send('9')
                    client.expect('account name')
                client.send(ACCOUNTS[name])
                client.expect('enter your password')
                client.send(journey.PASSWORD)
                client.expect('PRESS RETURN')
                client.send('')
                client.expect('Please select an option')
                client.send('1')
                menu = client.expect('Which character would you like to play?')
                match = re.search(r'\|\s*(\d+)\s*\|\s*' + name + r'\s*\|', menu)
                assert match, menu
                client.send(match.group(1))
                client.expect('Play as')
                client.send('y')
                client.expect('Welcome to New Duris', timeout=60)
                client.expect('<>', timeout=20)
                command(client, 'toggle paging no')
                return client

            def act(name, text, expected):
                evidence['last_command'] = name + ': ' + text
                client = clients[name]
                start = len(client.transcript)
                client.send(text)
                client.expect(expected, timeout=60)
                if expected != 'Please select an option':
                    command(client, 'score')
                return bytes(client.transcript[start:]).decode(errors='replace')

            def check(label):
                evidence['checks'].append(label)
                print('PASS ' + label, flush=True)

            def bank(room=BANK):
                god = clients[GOD]
                command(god, f'goto {room}')
                command(god, 'setbit room here flag SAFE 1')
                command(god, 'load obj 3097')
                for name in (OWNER, VISITOR, OTHER):
                    if name in clients:
                        assert 'Ok.' in command(god, 'transfer ' + name)

            def enter(name, target='', items=1, title=None):
                deadline = time.monotonic() + 60
                client = clients[name]
                while True:
                    start = len(client.transcript)
                    client.send(('enter locker ' + target).strip())
                    match, text = client.expect_any(('this cost you', DENIED, 'Please try later',
                                                    'Slow your roll', 'could not be opened'), timeout=30)
                    if match == 'this cost you':
                        if items is not None:
                            assert f'You have {items} items' in text, text
                        if not target.lower().startswith('guild'):
                            assert ACCOUNTS[OWNER].lower() not in text.lower(), text
                            own = name == OWNER or not target or target.lower().startswith('account.' + ACCOUNTS[name].lower() + '.')
                            expected_title = name if own else (title or (ALT if target.lower().startswith('account.') else target.lower().removesuffix('.locker').capitalize()))
                            assert 'The Storage Locker for ' + expected_title in text, text
                        command(client, 'score')
                        return bytes(client.transcript[start:]).decode(errors='replace')
                    assert match in ('Please try later', 'Slow your roll'), text
                    assert time.monotonic() < deadline, 'locker save never finished'
                    command(client, 'score')
                    time.sleep(.1)

            def leave(name):
                command(clients[name], 'open door')
                act(name, 'north', 'applying magic locks')

            def denied(name, target=OWNER, expected=DENIED):
                response = act(name, 'enter locker ' + target, expected)
                assert 'currently using' not in response and 'trying to get in' not in response, response
                assert 'Slow your roll' not in response and 'this cost you' not in response, response

            def contents():
                return sql("SELECT l.locker_name,li.obj_uid,li.vnum FROM lockers l "
                           "JOIN locker_items li ON li.locker_id=l.id "
                           f"WHERE l.locker_name='{ACCOUNT_LOCKER}' "
                           "ORDER BY l.locker_name,li.obj_uid")

            def payload():
                fields = ','.join('li.' + name for name in (
                    'vnum','weight','cost','timer','extra_flags','wear_flags','item_type',
                    'value0','value1','value2','value3','value4','value5','value6','value7',
                    'item_condition','bitvector1','bitvector2','bitvector3','bitvector4','bitvector5','item_material'))
                result = {
                    'items': sql("SELECT li.obj_uid,COALESCE(parent.obj_uid,0),c.chest_name," + fields +
                        ",HEX(li.name),HEX(li.short_descr),HEX(li.description),HEX(li.action_descr) "
                        "FROM locker_items li JOIN lockers l ON l.id=li.locker_id "
                        "JOIN private_chests c ON c.id=li.chest_id LEFT JOIN locker_items parent ON parent.id=li.container_id "
                        f"WHERE l.locker_name='{ACCOUNT_LOCKER}' ORDER BY li.obj_uid"),
                    'affects': sql("SELECT li.obj_uid,a.location,a.modifier FROM locker_item_affects a "
                        "JOIN locker_items li ON li.id=a.item_id JOIN lockers l ON l.id=li.locker_id "
                        f"WHERE l.locker_name='{ACCOUNT_LOCKER}' ORDER BY li.obj_uid,a.location,a.modifier"),
                    'descriptions': sql("SELECT li.obj_uid,HEX(e.keyword),HEX(e.description) FROM locker_item_extra_descr e "
                        "JOIN locker_items li ON li.id=e.item_id JOIN lockers l ON l.id=li.locker_id "
                        f"WHERE l.locker_name='{ACCOUNT_LOCKER}' ORDER BY li.obj_uid,e.keyword,e.description")}
                evidence['latest_locker_payload'] = result
                return result

            try:
                import_help(env, runtime, sql)
                boot()
                for name, account in ACCOUNTS.items():
                    client = journey.MudClient(plain)
                    clients[name] = client
                    journey.create_character(client, expected_room=None, account=account,
                                             character=name, email=name.lower() + '@example.invalid')
                    act(name, 'save', 'Save complete for ' + name + '.')
                stop()
                sql(f"UPDATE player_data SET level=62,highest_level=62 WHERE name='{GOD}'")
                sql(f"UPDATE player_data SET level=20,highest_level=20,platinum=2000 "
                    f"WHERE name IN ('{OWNER}','{VISITOR}','{OTHER}')")
                # Two alts on the same account exercise title fallback and faction filtering.
                for name in (ALT, EVIL_ALT):
                    sql(f"UPDATE player_data SET account_name='{ACCOUNTS[OWNER]}' WHERE name='{name}'")
                    sql(f"UPDATE account_characters SET account_name='{ACCOUNTS[OWNER]}',last_login=DATE_ADD(NOW(),INTERVAL 1 DAY) WHERE char_name='{name}'")
                sql(f"UPDATE player_data SET racewar=2 WHERE name='{EVIL_ALT}'")
                sql(f"UPDATE account_characters SET racewar=2,last_login=DATE_ADD(NOW(),INTERVAL 2 DAY) WHERE char_name='{EVIL_ALT}'")
                boot()
                for name in ACTIVE:
                    login(name)
                bank()
                # Keep the fixture funds in the bank. Carrying thousands of
                # loose coins invokes accidental drops, including inside lockers.
                for name in (OWNER, VISITOR, OTHER):
                    act(name, 'deposit all', 'Your account contains:')
                assert sql(f"SELECT last_login > NOW() FROM account_characters WHERE char_name='{ALT}'") == '1'
                for name in (OWNER, VISITOR, OTHER):
                    assert sql(f"SELECT level FROM player_data WHERE name='{name}'") == '20'
                check('separate owner/visitor accounts and three level-20 mortal actors at a real bank')
                assert sql(f"SELECT COUNT(*) FROM lockers WHERE locker_name='{ACCOUNT_LOCKER}'") == '0'
                denied(VISITOR)
                denied(OTHER, 'account.Otheracct.2')
                denied(OTHER, 'account.Otheracct.1junk', expected='could not be opened')
                denied(OTHER, 'NoSuchCharacter', expected='could not be opened')
                assert sql("SELECT COUNT(*) FROM lockers WHERE locker_name LIKE 'account.%'") == '0'
                enter(VISITOR, 'AcCoUnT.Guestacct.1.LoCkEr', items=0)
                leave(VISITOR)
                assert sql("SELECT COUNT(*) FROM lockers WHERE locker_name='account.guestacct.1.locker'") == '1'
                # The live starter-kit catalog may already supply a mace. Keep the
                # two fixture lockers' marker counts independent of that catalog.
                command(clients[OWNER], 'drop all.mace')
                assert 'mace' not in command(clients[OWNER], 'inventory').lower()
                for _ in range(1):
                    command(clients[GOD], 'load obj 677')
                    act(GOD, 'give mace ' + OWNER, 'Ok.')
                enter(OWNER, OWNER, items=0)
                assert sql(f"SELECT COUNT(*) FROM lockers WHERE locker_name='{ACCOUNT_LOCKER}'") == '1'
                check('first named and mixed-case canonical owner entry creates only the authenticated account side')
                act(OWNER, 'drop mace', 'You drop')
                act(OWNER, 'grant add ' + VISITOR, 'given access')
                help_text = act(OWNER, 'help locker', 'one personal locker per faction')
                assert '<character>.locker' in help_text and 'account.<account>.<side>' in help_text
                assert "account's" in act(OWNER, 'help grant', "account's")

                # Authorization is checked before exposing the occupied account locker.
                occupied = act(VISITOR, 'enter locker ' + OWNER, 'currently using that locker')
                assert OWNER in occupied
                denied(OTHER)
                leave(OWNER)
                denied(OTHER)  # Also safe while the owner's terminal locker save may be pending.
                enter(VISITOR, 'VaUlToWnEr')
                leave(VISITOR)
                check('mortal character grant -> leave -> mixed-case named account entry; occupancy privacy')

                enter(VISITOR, 'AlToWnEr')
                leave(VISITOR)
                enter(VISITOR, OWNER.lower() + '.locker')
                leave(VISITOR)
                assert sql("SELECT COUNT(*) FROM lockers WHERE locker_name NOT LIKE 'account.%' AND locker_name NOT LIKE 'guild.%'") == '0'
                check('character aliases share one faction locker, use the requested character title and create no player locker')

                enter(OWNER, OWNER)
                act(OWNER, 'grant rem ' + VISITOR, 'lost access')
                leave(OWNER)
                denied(VISITOR)
                enter(OWNER, 'account.Ownervault.1.locker')
                act(OWNER, 'grant add ' + ACCOUNTS[VISITOR], 'given access')
                # Save a valid same-side character grant, then switch that character's side offline below.
                act(OWNER, 'grant add ' + OTHER, 'given access')
                leave(OWNER)
                enter(VISITOR, 'account.Ownervault.1')
                leave(VISITOR)
                enter(VISITOR, 'account.Ownervault.1.locker')
                leave(VISITOR)
                check('named and canonical owner entry, revocation, account visitor grant, optional canonical suffix')

                act(VISITOR, 'save', 'Save complete for ' + VISITOR + '.')
                command(clients[GOD], 'goto 16558')  # The real Woodseer inn handles mortal logout.
                assert 'Ok.' in command(clients[GOD], 'transfer ' + VISITOR)
                act(VISITOR, 'rent', 'Please select an option')
                clients.pop(VISITOR).close()
                login(VISITOR)
                command(clients[GOD], f'goto {BANK}')
                assert 'Ok.' in command(clients[GOD], 'transfer ' + VISITOR)
                enter(VISITOR, OWNER)
                leave(VISITOR)
                check('account grant and named entry after visitor reconnect')

                before_restart = contents()
                assert len(before_restart.splitlines()) == 1, before_restart
                grants = sql("SELECT owner,visitor FROM locker_access ORDER BY owner,visitor")
                evidence['contents_before_restart'] = before_restart
                evidence['grants_before_restart'] = grants
                stop()
                sql(f"UPDATE player_data SET racewar=2 WHERE name='{OTHER}'")
                sql(f"UPDATE account_characters SET racewar=2 WHERE char_name='{OTHER}'")
                sql(f"UPDATE accounts SET last_good_char=NULL WHERE account_name='{ACCOUNTS[OTHER]}'")
                boot()
                for name in ACTIVE:
                    login(name)
                bank(NEUTRAL_COUNTER)
                assert contents() == before_restart
                assert sql("SELECT owner,visitor FROM locker_access ORDER BY owner,visitor") == grants
                for target in (OWNER, OWNER + '.locker', 'account.Ownervault.1', ACCOUNT_LOCKER):
                    denied(OTHER, target)
                denied(OWNER, EVIL_ALT)
                denied(VISITOR, EVIL_ALT)
                denied(VISITOR, 'account.Ownervault.2')
                enter(OWNER)
                denied(OTHER)  # No opposite-side occupant disclosure either.
                leave(OWNER)
                enter(VISITOR, OWNER)
                assert 'mace' in command(clients[VISITOR], 'look in chest')
                leave(VISITOR)
                check('cold restart preserves contents and grants; opposite-side aliases and own-account access denied')

                enter(OWNER)
                act(OWNER, 'grant rem ' + ACCOUNTS[VISITOR], 'lost access')
                leave(OWNER)
                denied(VISITOR)
                assert contents() == before_restart
                check('account-grant revocation; contents and item identities unchanged')

                # Add a nested container plus a password-protected private chest.
                command(clients[GOD], 'load obj 83181')
                act(GOD, 'give alatorin ' + OWNER, 'Ok.')
                for _ in range(2):
                    command(clients[GOD], 'load obj 677')
                    act(GOD, 'give mace ' + OWNER, 'Ok.')
                command(clients[OWNER], 'open alatorin')
                act(OWNER, 'put mace alatorin', 'Ok.')
                enter(OWNER)
                act(OWNER, 'drop alatorin', 'You drop')
                act(OWNER, 'eq chest create secret chestpass', "Private chest 'secret' created")
                leave(OWNER)
                enter(OWNER, items=None)
                act(OWNER, 'open secret', "You open the 'secret' chest")
                act(OWNER, 'put mace secret', 'Ok.')
                act(OWNER, 'close', 'You close the chest')
                act(OWNER, 'save', 'Save complete for ' + OWNER + '.')
                # Wait for the asynchronous checkpoint, then capture durable payload.
                deadline = time.monotonic() + 30
                while len(contents().splitlines()) != 4:
                    assert time.monotonic() < deadline, contents()
                    time.sleep(.1)
                assert sql(f"SELECT COUNT(*) FROM locker_items li JOIN lockers l ON li.locker_id=l.id WHERE l.locker_name='{ACCOUNT_LOCKER}' AND li.container_id IS NOT NULL") == '1'
                assert sql(f"SELECT COUNT(*) FROM locker_items li JOIN private_chests c ON c.id=li.chest_id JOIN lockers l ON li.locker_id=l.id WHERE l.locker_name='{ACCOUNT_LOCKER}' AND c.is_public=0") == '1'
                before_restart = contents()
                chest_metadata = sql(f"SELECT c.chest_name,c.password_hash,c.is_public,c.sort_config FROM private_chests c JOIN lockers l ON l.id=c.locker_id WHERE l.locker_name='{ACCOUNT_LOCKER}' ORDER BY c.id")
                assert 'secret' in chest_metadata
                saved_payload = payload()
                assert saved_payload['descriptions'], 'nested backpack extra description was not saved'
                evidence['saved_locker_payload'] = saved_payload
                assert sql(f"SELECT race,owner_pid IS NULL FROM lockers WHERE locker_name='{ACCOUNT_LOCKER}'") == '0\t1'
                assert sql("SELECT COUNT(*) FROM player_data WHERE name LIKE '%.locker'") == '0'
                check('dedicated saves retain public, private and nested items without player metadata')
                # Fault injection rejects locker item writes, including private chests.
                # The save must roll back and normal shutdown must remain live.
                sql("CREATE TRIGGER locker_journey_fail BEFORE INSERT ON locker_items FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='locker journey injected failure'")
                process.send_signal(signal.SIGTERM)
                clients[OWNER].expect('Shutdown cancelled because a locker save failed.', timeout=60)
                assert process.poll() is None
                assert contents() == before_restart
                sql('DROP TRIGGER locker_journey_fail')
                check('failed locker writes roll back and cancel shutdown while keeping the live recovery source')
                # Do not leave the locker or issue player checkpoints before shutdown.
                stop(checkpoints=False)
                assert contents() == before_restart
                assert payload() == saved_payload, json.dumps(evidence['latest_locker_payload'], indent=2)
                assert not list(runtime.rglob('*.locker')), 'unexpected player-format locker file'
                boot()
                for name in ACTIVE:
                    login(name)
                bank(NEUTRAL_COUNTER)
                enter(OWNER, items=None)
                assert 'mace' in command(clients[OWNER], 'look in alatorin')
                act(OWNER, 'open secret', "You open the 'secret' chest")
                assert 'mace' in command(clients[OWNER], 'look in secret')
                act(OWNER, 'close', 'You close the chest')
                leave(OWNER)
                # A terminal save may still be pending; shutdown must drain it.
                stop(checkpoints=False)
                assert contents() == before_restart
                assert sql(f"SELECT c.chest_name,c.password_hash,c.is_public,c.sort_config FROM private_chests c JOIN lockers l ON l.id=c.locker_id WHERE l.locker_name='{ACCOUNT_LOCKER}' ORDER BY c.id") == chest_metadata
                assert payload() == saved_payload, json.dumps(evidence['latest_locker_payload'], indent=2)
                boot()
                for name in ACTIVE:
                    login(name)
                bank(NEUTRAL_COUNTER)
                check('occupied and just-left shutdown/restart preserve public/private/nested item UIDs, chest credentials and metadata without player files')

                # Guild creation requires level 25; the actor remains a mortal.
                command(clients[GOD], f'setbit char {OWNER} level 30')
                act(OWNER, 'save', 'Save complete for ' + OWNER + '.')
                assert sql(f"SELECT level FROM player_data WHERE name='{OWNER}'") == '30'
                act(GOD, f'supervise found {OWNER} n Lockerjourney', 'new association is set up')
                deadline = time.monotonic() + 30
                while True:
                    guild_id = sql("SELECT id FROM guilds WHERE TRIM(name)='Lockerjourney'")
                    if guild_id.isdecimal():
                        break
                    assert time.monotonic() < deadline, 'new guild snapshot never landed'
                    time.sleep(.1)
                guild_target = f'guild.{guild_id}'
                act(GOD, f'supervise setbit {guild_id} prestige 500', 'prestige points to 500')
                denied(VISITOR, guild_target + '.locker')
                enter(OWNER, guild_target.upper() + '.LoCkEr', items=0)
                denied(VISITOR, guild_target + '.locker')
                leave(OWNER)
                command(clients[GOD], 'load obj 677')
                act(GOD, 'give mace ' + OWNER, 'Ok.')
                enter(OWNER, 'guild', items=0)
                act(OWNER, 'drop mace', 'You drop')
                leave(OWNER)
                for target in (guild_target, guild_target.upper() + '.LoCkEr', 'guild'):
                    response = enter(OWNER, target, items=1)
                    assert '2 copper' in response, response
                    leave(OWNER)
                act(GOD, f'supervise setbit {guild_id} prestige 0', 'prestige points to 0')
                for target in ('guild', guild_target, guild_target + '.locker'):
                    act(OWNER, 'enter locker ' + target, 'not yet prestigious enough')
                act(GOD, f'supervise setbit {guild_id} prestige 500', 'prestige points to 500')
                denied(OWNER, f'guild.{int(guild_id) + 1}.locker')
                act(GOD, f'supervise govern {guild_id}', 'You are now a god of')
                act(GOD, 'society punish ' + OWNER, 'on parole')
                for target in ('guild', guild_target, guild_target + '.locker'):
                    act(OWNER, 'enter locker ' + target, 'Try becoming part of a guild first')
                assert sql(f"SELECT COUNT(*) FROM lockers WHERE locker_name='{guild_target}.locker'") == '1'
                assert sql(f"SELECT COUNT(*) FROM locker_access WHERE owner='{guild_target}.locker'") == '0'
                assert contents() == before_restart
                check('canonical guild membership, creation, contents and fees match guild; nonmember/prestige/parole gates hold')
                stop()
                target = ROOT / 'bin/analysis/issue-234'
                target.mkdir(parents=True, exist_ok=True)
                (target / 'journey.json').write_text(json.dumps(evidence, indent=2) + '\n')
            except Exception as error:
                target = ROOT / 'bin/analysis/issue-234' / f'failure-{time.time_ns()}'
                target.mkdir(parents=True)
                (target / 'evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
                shutil.copy2(output_path, target / 'server.out')
                shutil.copytree(runtime / 'logs', target / 'logs')
                traces = '\n'.join(name + ':\n' + bytes(client.transcript[-5000:]).decode(errors='replace')
                                   for name, client in clients.items())
                raise AssertionError(f'{error}\nEvidence: {target}\n{traces}') from error
            finally:
                for client in clients.values():
                    client.close()
                if process and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=10)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--server', type=Path, default=ROOT / 'bin/server/dms_new')
    args = parser.parse_args()
    if not all(os.getenv(key) for key in ('TEST_DB_HOST', 'TEST_DB_USER', 'TEST_DB_PASSWORD')):
        parser.error('requires explicit disposable TEST_DB_HOST, TEST_DB_USER and TEST_DB_PASSWORD')
    run(args.server.resolve())
