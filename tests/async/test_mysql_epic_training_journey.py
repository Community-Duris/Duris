#!/usr/bin/env python3
"""Actual epic training and imported help on a disposable loopback MariaDB schema.

Requires an already generated full world, a matching server binary and explicit
TEST_DB_HOST/USER/PASSWORD. Never reads the checkout .env. The importer is copied
with tracked content to a temporary directory before use.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey
from contextlib import contextmanager
import uuid
from run_world_quest_dual_backend import setup_run_root

ROOT = journey.ROOT
PLAYER, ACCOUNT = 'Trainpupil', 'Trainplayer'
GOD, GOD_ACCOUNT = 'Trainwarden', 'Trainstaff'
TOUGHNESS = 1230


@contextmanager
def authority(runtime):
    host = os.environ['TEST_DB_HOST']
    assert host in ('127.0.0.1', 'localhost'), 'use a disposable loopback database'
    port = os.environ.get('TEST_DB_PORT', '3306')
    schema = 'epic_help_journey_' + uuid.uuid4().hex[:12]
    env = dict(PATH=os.environ.get('PATH', '/usr/bin:/bin'), ENVIRONMENT='local',
               DB_HOST=host, DB_PORT=port, DB_USER=os.environ['TEST_DB_USER'],
               DB_PASSWD=os.environ['TEST_DB_PASSWORD'], DB_NAME=schema,
               DB_ALLOWED_TARGETS=host + '/' + schema, DB_TLS='FALSE', DB_SOCKET='',
               MYSQL_PWD=os.environ['TEST_DB_PASSWORD'], PERSISTENCE_MODE='mariadb-primary',
               REDIS='FALSE', CHAOS_MUD='FALSE', LISTEN_ADDRESS='127.0.0.1',
               DURIS_WEBSOCKET_LISTEN_ADDRESS='127.0.0.1')
    mysql = ['mysql', '--protocol=tcp', '-h', host, '-P', port, '-u', env['DB_USER'], '-N', '-B']

    def sql(statement, selected=True):
        return subprocess.check_output(mysql + ([schema] if selected else []),
                                       input=statement, text=True, env=env).strip()

    sql('CREATE DATABASE ' + schema + ' CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci', False)
    try:
        sql((ROOT / 'migrations/bootstrap_multithread_safe.sql').read_text())
        for action in (('adopt', '--kind', 'fresh_bootstrap'), ('run',)):
            subprocess.run(['python3', 'scripts/migration_runner.py', *action],
                           cwd=ROOT, env=env, check=True)
        yield env, sql
    finally:
        sql('DROP DATABASE ' + schema, False)


def command(client, text):
    client.send(text)
    # Epic purchases complete asynchronously; wait for the actual outcome
    # before querying the resulting score.
    result = ''
    if text == 'practice toughness':
        _, result = client.expect_any(('You improve your mastery of', 'You have mastered',
                                       'You must reach level', 'already mastered this skill'),
                                      timeout=30)
    tail = client.expect('<>', timeout=30)
    result += tail
    client.send('score')
    result += client.expect('Score information for', timeout=30)
    tail = client.expect('<>', timeout=30)
    return result + tail


def import_help(env, runtime, sql):
    staging = runtime / 'help-import'
    (staging / 'scripts').mkdir(parents=True)
    shutil.copy2(ROOT / 'scripts/import_help_to_prod.sh', staging / 'scripts')
    shutil.copytree(ROOT / 'lib/information', staging / 'lib/information')
    shutil.copytree(ROOT / 'help', staging / 'help')
    result = subprocess.run(['bash', str(staging / 'scripts/import_help_to_prod.sh'), '--local'],
                            input='yes\n', text=True, env=env, capture_output=True, timeout=120)
    assert result.returncode == 0, result.stdout + result.stderr
    titles = sql("SELECT title FROM pages WHERE UPPER(title) IN ('QUICK STEP','QUICKSTEP','SEADOG') ORDER BY title")
    assert set(titles.upper().splitlines()) == {'QUICK STEP', 'QUICKSTEP', 'SEADOG'}, titles
    print('PASS maintained help importer publishes all three real topics to MariaDB', flush=True)
    return {'topics': titles.splitlines(), 'script_sha256': hashlib.sha256(
            (ROOT / 'scripts/import_help_to_prod.sh').read_bytes()).hexdigest()}


def check_help(client):
    response = command(client, 'help ferries')
    for name in ('WaveDancer', 'Seaspray', 'Stalval', 'Crimson Fulgur',
                 'Ironhold Grudge', 'Old Rickety Ferry', 'Stromvok', 'Menden-on-the-Deep'):
        assert name in response, (name, response)
    assert 'SeaStalker' not in response and 'Fenaline' not in response, response
    for topic in ('quickstep', 'QuIcKsTeP', 'quick step'):
        response = command(client, 'help ' + topic)
        assert 'powerful blow' in response and 'Redirect:' not in response, response
    response = command(client, 'help seadog')
    for fragment in ('passive seafaring', 'owner with Seadog', 'anywhere aboard',
                     '10% more proceeds', 'Swashbuckler', 'level 30'):
        assert fragment in response, (fragment, response)
    print('PASS actual SQL help command: ferries, redirect, mixed case, spaced topic and Seadog', flush=True)


def stats(text):
    # Staff changes and GMCP negotiation can leave earlier score text in the
    # stream. Assert the final score requested after the command's completion.
    text = text.rsplit('Score information for', 1)[-1]
    hp = re.search(r'Hit points:\s*(\d+)\((\d+)\)', text)
    epics = re.search(r'Epic points(?:\(total\))?:\s*(\d+)', text)
    coins = re.search(r'Coins carried:\s*(\d+) platinum\s+(\d+) gold\s+(\d+) silver\s+(\d+) copper', text)
    assert hp and epics and coins, text
    return {'hp': int(hp[1]), 'max_hp': int(hp[2]), 'epics': int(epics[1]),
            'coins': sum(int(value) * scale for value, scale in zip(coins.groups(), (1000, 100, 10, 1)))}


def run(binary):
    (ROOT / 'bin/tests').mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='epic-training-', dir=ROOT / 'bin/tests') as temporary:
        runtime = Path(temporary)
        players, critical, output_path = setup_run_root(runtime)
        (runtime / 'help').symlink_to(ROOT / 'help', target_is_directory=True)
        (runtime / 'bin/server').mkdir(parents=True)
        executable = runtime / 'bin/server/dms'
        shutil.copy2(binary, executable)
        executable.chmod(0o700)
        plain, tls, websocket = journey.available_ports()
        clients = []
        process = None
        evidence = {'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(), 'purchases': []}
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
                    assert process.poll() is None and time.monotonic() < deadline, 'training world failed to boot'
                    time.sleep(.1)

            def stop():
                for client in clients:
                    client.close()
                clients.clear()
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=60)
                assert process.returncode == 0

            def login(name, account, reconnecting=False):
                client = journey.MudClient(plain)
                clients.append(client)
                entry, _ = client.expect_any(('term type', 'account name'))
                if entry == 'term type':
                    client.send('9')
                    client.expect('account name')
                client.send(account)
                client.expect('enter your password')
                client.send(journey.PASSWORD)
                client.expect('PRESS RETURN')
                client.send('')
                client.expect('Please select an option')
                client.send('1')
                client.expect(name)
                client.send('1')
                if reconnecting:
                    # Link-dead characters reattach immediately; cold loads
                    # instead ask the normal "Play as" confirmation.
                    client.expect('Reconnecting.', timeout=30)
                else:
                    client.expect('Play as')
                    client.send('y')
                    client.expect('Welcome to New Duris', timeout=60)
                client.expect('<>', timeout=20)
                command(client, 'toggle paging no')
                return client

            def saved_skill():
                value = sql(f"SELECT learned,taught FROM player_skills WHERE pid=(SELECT pid FROM player_data WHERE name='{PLAYER}') AND skill_id={TOUGHNESS}")
                return tuple(map(int, value.split('\t'))) if value else (0, 0)

            def saved_player():
                return sql(f"SELECT level,epics,epic_revision,platinum,gold,silver,copper,hit_diff FROM player_data WHERE name='{PLAYER}'")

            def ledger():
                return sql(f"SELECT epic_revision,delta,balance_after,reason_type,reason_id FROM epic_ledger WHERE pid=(SELECT pid FROM player_data WHERE name='{PLAYER}') ORDER BY epic_revision")

            def save():
                player.send('save')
                player.expect('Save complete for ' + PLAYER + '.', timeout=60)
                player.expect('<>', timeout=20)

            try:
                evidence['help_import'] = import_help(env, runtime, sql)
                boot()
                for name, account in ((PLAYER, ACCOUNT), (GOD, GOD_ACCOUNT)):
                    client = journey.MudClient(plain)
                    clients.append(client)
                    journey.create_character(client, expected_room=None, account=account,
                                             character=name, email=name.lower() + '@example.invalid')
                    client.send('save')
                    client.expect('Save complete for ' + name + '.', timeout=60)
                    client.expect('<>', timeout=20)
                stop()
                sql(f"UPDATE player_data SET level=62,highest_level=62 WHERE name='{GOD}'")
                sql(f"UPDATE player_data SET level=55,highest_level=57,epics=10000,platinum=12000,gold=0,silver=0,copper=0 WHERE name='{PLAYER}'")
                sql(f"UPDATE epic_balance_baseline b JOIN player_data p ON p.pid=b.pid SET b.opening_balance=p.epics,b.opening_revision=p.epic_revision WHERE p.name='{PLAYER}'")
                assert not ledger(), 'training fixture must start without epic ledger operations'
                sql(f"DELETE FROM player_skills WHERE pid=(SELECT pid FROM player_data WHERE name='{PLAYER}') AND skill_id={TOUGHNESS}")
                boot()
                god, player = login(GOD, GOD_ACCOUNT), login(PLAYER, ACCOUNT)
                check_help(player)
                command(god, 'goto 1')
                command(god, 'setbit room here flag SAFE 0')
                command(god, 'setbit room here flag NO_HEAL 1')
                command(god, 'transfer ' + PLAYER)
                command(god, 'load mob 80862')
                assert 'butcher' in command(player, 'look'), 'the actual toughness teacher is absent'
                player.socket.sendall(bytes([255, 253, 201]))  # negotiate GMCP
                before = stats(command(player, 'score'))
                refused = command(player, 'practice toughness')
                assert 'must reach level 56' in refused, refused
                assert stats(refused) == before
                save()
                assert saved_skill() == (0, 0)
                assert not ledger(), 'refused purchase created an epic operation'
                evidence['below_minimum'] = before
                print('PASS actual teacher at 55: refusal preserves skill and both balances', flush=True)

                for level, learned, missing in ((56, 10, 0), (57, 20, 30)):
                    command(god, f'setbit char {PLAYER} level {level}')
                    initial = stats(command(player, 'score'))
                    command(god, f'setbit char {PLAYER} hit {initial["max_hp"] - missing}')
                    before = stats(command(player, 'score'))
                    assert before['max_hp'] - before['hp'] == missing, before
                    packet_start = len(player.transcript)
                    purchased = command(player, 'practice toughness')
                    after = stats(purchased)
                    assert 'improve your mastery of' in purchased, purchased
                    assert after['epics'] == before['epics'] - 150
                    assert after['coins'] == before['coins'] - 1000000
                    save()
                    assert saved_skill() == (learned, learned), saved_skill()
                    assert len(ledger().splitlines()) == learned // 10
                    evidence['purchases'].append({'level': level, 'before': before, 'after': after, 'learned': learned})
                    assert after['max_hp'] == before['max_hp'] + 10, evidence['purchases'][-1]
                    assert after['max_hp'] - after['hp'] == missing, evidence['purchases'][-1]
                    packets = re.findall(rb'\xff\xfa\xc9Char\.Vitals (.*?)\xff\xf0',
                                         bytes(player.transcript[packet_start:]), re.S)
                    assert packets, 'training did not publish GMCP vitals'
                    vitals = json.loads(packets[-1])
                    assert vitals['maxHp'] == after['max_hp'] and vitals['hp'] == after['hp'], vitals
                    evidence['purchases'][-1]['gmcp'] = vitals
                    time.sleep(1)
                    assert stats(command(player, 'score')) == after, 'HP changed on a later pulse'
                    # setbit schedules the existing affect refresh even for an unchanged hit value.
                    for _ in range(2):
                        command(god, f'setbit char {PLAYER} hit {after["hp"]}')
                        assert stats(command(player, 'score')) == after, 'refresh stacked the toughness bonus'
                    print(f'PASS actual teacher at {level}: one charge, learned {learned}, prompt/GMCP HP +10, missing HP {missing}, stable refreshes', flush=True)

                command(god, f"setbit char {PLAYER} skill 'toughness' 95")
                command(god, f"setbit char {PLAYER} taught 'toughness' 95")
                before = stats(command(player, 'score'))
                mastered = command(player, 'practice toughness')
                after = stats(mastered)
                assert 'You have mastered' in mastered, mastered
                assert after['epics'] == before['epics'] - 600
                assert after['coins'] == before['coins'] - 4000000
                assert after['max_hp'] == before['max_hp'] + 5
                save()
                assert saved_skill() == (100, 100)
                repeated = command(player, 'practice toughness')
                assert 'already mastered this skill' in repeated, repeated
                assert stats(repeated) == after
                save()
                before_restart = saved_player()
                ledger_before_restart = ledger()
                assert len(ledger_before_restart.splitlines()) == 3
                evidence['mastery'] = after
                player.close()
                clients.remove(player)
                time.sleep(.5)
                player = login(PLAYER, ACCOUNT, reconnecting=True)
                assert stats(command(player, 'score')) == after
                assert saved_skill() == (100, 100) and ledger() == ledger_before_restart
                evidence['live_reconnect'] = True
                stop()
                boot()
                player = login(PLAYER, ACCOUNT)
                check_help(player)
                restored = stats(command(player, 'score'))
                assert restored['max_hp'] == after['max_hp'], (restored, after)
                assert restored['max_hp'] - restored['hp'] == 30, restored
                assert restored['epics'] == after['epics'] and restored['coins'] == after['coins']
                assert saved_skill() == (100, 100)
                assert saved_player() == before_restart
                assert ledger() == ledger_before_restart
                print('PASS mastery/at-cap refusal and clean restart preserve skill, balances and derived HP', flush=True)

                # The Illithid HP formula has a separate base-plus-skill branch.
                god = login(GOD, GOD_ACCOUNT)
                command(god, 'goto 1')
                command(god, 'setbit room here flag SAFE 0')
                command(god, 'setbit room here flag NO_HEAL 1')
                command(god, 'transfer ' + PLAYER)
                command(god, 'load mob 80862')
                command(god, f'setbit char {PLAYER} class psionicist')
                command(god, f'setbit char {PLAYER} race illithid')
                command(god, f"setbit char {PLAYER} skill 'toughness' 0")
                command(god, f"setbit char {PLAYER} taught 'toughness' 0")
                player.socket.sendall(bytes([255, 253, 201]))
                for learned, missing in ((10, 0), (20, 30)):
                    initial = stats(command(player, 'score'))
                    command(god, f'setbit char {PLAYER} hit {initial["max_hp"] - missing}')
                    before = stats(command(player, 'score'))
                    packet_start = len(player.transcript)
                    purchased = command(player, 'practice toughness')
                    assert 'improve your mastery of' in purchased, purchased
                    after = stats(purchased)
                    assert after['max_hp'] == before['max_hp'] + 5, (before, after)
                    assert after['max_hp'] - after['hp'] == missing, (before, after)
                    assert after['epics'] == before['epics'] - 150
                    assert after['coins'] == before['coins'] - 1000000
                    packets = re.findall(rb'\xff\xfa\xc9Char\.Vitals (.*?)\xff\xf0',
                                         bytes(player.transcript[packet_start:]), re.S)
                    assert packets, 'Illithid training did not publish GMCP vitals'
                    vitals = json.loads(packets[-1])
                    assert vitals['maxHp'] == after['max_hp'] and vitals['hp'] == after['hp'], vitals
                    for _ in range(2):
                        command(god, f'setbit char {PLAYER} hit {after["hp"]}')
                        assert stats(command(player, 'score')) == after
                    save()
                    assert saved_skill() == (learned, learned)
                    assert len(ledger().splitlines()) == 3 + learned // 10
                    evidence['purchases'].append({'race': 'illithid', 'before': before, 'after': after,
                                                 'learned': learned, 'gmcp': vitals})
                before_restart = saved_player()
                ledger_before_restart = ledger()
                stop()
                boot()
                player = login(PLAYER, ACCOUNT)
                check_help(player)
                assert stats(command(player, 'score')) == after
                assert saved_skill() == (20, 20)
                assert saved_player() == before_restart
                assert ledger() == ledger_before_restart
                evidence['epic_ledger'] = ledger_before_restart.splitlines()
                stop()
                target = ROOT / 'bin/analysis/epic-training'
                target.mkdir(parents=True, exist_ok=True)
                (target / 'journey.json').write_text(json.dumps(evidence, indent=2) + '\n')
                print('PASS Illithid full/injured HP, repeated refreshes, GMCP and clean restart', flush=True)
            except Exception as error:
                target = ROOT / 'bin/analysis/epic-training' / f'failure-{time.time_ns()}'
                target.mkdir(parents=True)
                (target / 'evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
                shutil.copy2(output_path, target / 'server.out')
                shutil.copytree(runtime / 'logs', target / 'logs')
                traces = '\n'.join(bytes(client.transcript[-5000:]).decode(errors='replace') for client in clients)
                raise AssertionError(f'{error}\nEvidence: {target}\n{traces}') from error
            finally:
                for client in clients:
                    client.close()
                if process and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        process.kill(); process.wait(timeout=10)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--server', type=Path, default=ROOT / 'bin/server/dms_new')
    args = parser.parse_args()
    if not all(os.getenv(key) for key in ('TEST_DB_HOST', 'TEST_DB_USER', 'TEST_DB_PASSWORD')):
        parser.error('requires explicit disposable TEST_DB_HOST, TEST_DB_USER and TEST_DB_PASSWORD')
    else:
        run(args.server.resolve())
