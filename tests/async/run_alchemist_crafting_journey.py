#!/usr/bin/env python3
"""Real server alchemist loot, supported crafts, copyover and player reload.

Uses the existing isolated combat journey's account, socket and world helpers.
Requires the combined #551/#661 server and the flat-file repository inspector.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey
from run_generated_npc_journey import FIXTURE, drain


def authored(vnum):
    for path in sorted((journey.ROOT / 'areas/obj').glob('*.obj')):
        match = re.search(rf'(?ms)^#{vnum}\n.*?(?=^#\d+\n|^\$~)', path.read_text())
        if match:
            return match.group(0)
    raise AssertionError(f'missing authored object {vnum}')


def run(binary, mode='file'):
    assert mode in ('file', 'redis')
    with tempfile.TemporaryDirectory(prefix='alchemist-crafting-journey-') as temporary:
        root = Path(temporary)
        state = root / 'state'
        runtime = root / 'runtime'
        state.mkdir(mode=0o700)
        (state / 'domains').mkdir(mode=0o700)
        runtime.mkdir()
        (runtime / 'logs/log').mkdir(parents=True)
        journey.make_fixture(runtime)
        journey.generate_certificate(runtime)
        path = runtime / 'areas_mini/mini.obj'
        objects = path.read_text()
        for vnum in (102, 470, 806, 808, 1251, 868, 866, 865, 863, 859,
                     857, 855, 853, 850, 400230, 400231, 400291):
            if not re.search(rf'(?m)^#{vnum}$', objects):
                objects = objects.replace('$~', authored(vnum) + '$~')
        # real_object() uses the maintained sorted prototype index.
        blocks = re.findall(r'(?ms)^#\d+\n.*?(?=^#\d+\n|^\$~)', objects)
        path.write_text(''.join(sorted(blocks, key=lambda block: int(block.split('\n', 1)[0][1:]))) + '$~\n')
        path = runtime / 'areas_mini/mini.mob'
        mobiles = path.read_text()
        for index in range(200):
            mobiles = mobiles.replace('$~',
                f'#{22810 + index}\nalc{index:02d} alchemist~\n'
                f'the fixture alchemist {index:02d}~\n'
                f'The fixture alchemist {index:02d} waits here.\n~\n~\n'
                '2 0 0 0 0 0 0 0 S\nPH 0 524288 -1\n'
                '6 0 100 1000d1+20000 1d1+0\n0.0.0.0 0\n8 8 0\n$~')
        mobiles = mobiles.replace('$~', '#400001\nharvester~\nThe Harvester~\n'
            'The Harvester waits here.\n~\n~\n3 0 0 0 0 0 0 0 S\nPH 0 0 -1\n'
            '60 0 0 100d1+10000 1d1+0\n0.0.0.0 0\n8 8 0\n$~')
        path.write_text(mobiles)
        path = runtime / 'areas_mini/mini.zon'
        zone = path.read_text()
        zone = '\n'.join(line for line in zone.split('\n') if '22800 ' not in line)
        zone = zone.replace('29999 0 0 6 11 1', '499999 0 0 6 11 1')
        resets = ''.join(f'M 0 {22810 + i} 1 22800 100 0 0 0\n' for i in range(200))
        zone = zone.replace('\nS\n', '\n' + resets + 'M 0 400001 1 22800 100 0 0 0\nS\n')
        path.write_text(zone)
        for name in ('players', 'critical'):
            (runtime / 'journals' / name).mkdir(parents=True, mode=0o700)
        (runtime / 'bin/server').mkdir(parents=True)
        for name in ('dms', 'dms_new'):
            shutil.copy2(binary, runtime / 'bin/server' / name)
        port, tls, ws = journey.available_ports()
        env = dict(PATH=os.environ.get('PATH', '/usr/bin:/bin'), ENVIRONMENT='local',
            PERSISTENCE_MODE='flatfile-primary', FLATFILE_STATE_DIR=str(state),
            PLAYER_SAVE_JOURNAL_DIR=str(runtime / 'journals/players'),
            CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / 'journals/critical'),
            REDIS='FALSE', CHAOS_MUD='FALSE', LISTEN_ADDRESS='127.0.0.1',
            DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(ws),
            DURIS_WEBSOCKET_LISTEN_ADDRESS='127.0.0.1', DURIS_NEVENT_TRACE_PLAYER='1')
        database = None
        if mode == 'redis':
            host = os.environ['TEST_DB_HOST']
            assert host in ('127.0.0.1', 'localhost')
            assert os.environ.get('TEST_DB_DISPOSABLE') == '1', 'disposable database opt-in required'
            database_port = int(os.environ.get('TEST_DB_PORT', '3306'))
            redis_port = int(os.environ.get('TEST_REDIS_PORT', '6379'))
            assert 1 <= database_port <= 65535 and 1 <= redis_port <= 65535
            # Keep the server's database-qualified advisory lock below MySQL's
            # 64-byte lock-name limit, including its fixed prefix.
            database = 'alchemy_test_' + uuid.uuid4().hex[:12]
            env.update(DB_HOST=host, DB_PORT=str(database_port), DB_NAME=database,
                DB_USER=os.environ['TEST_DB_USER'], DB_PASSWD=os.environ['TEST_DB_PASSWORD'],
                MYSQL_PWD=os.environ['TEST_DB_PASSWORD'], DB_TLS='FALSE',
                DB_ALLOWED_TARGETS=host + '/' + database, PERSISTENCE_MODE='mariadb-primary')
            mysql = ['mysql', '--protocol=tcp', '-h', host, '-P', str(database_port),
                     '-u', env['DB_USER'], '-N', '-B']
            def sql(statement, selected=True):
                return subprocess.check_output(mysql + ([database] if selected else []),
                    input=statement, text=True, env=env).strip()
            sql('CREATE DATABASE ' + database, False)
            sql((journey.ROOT / 'migrations/bootstrap_multithread_safe.sql').read_text())
            for arguments in [('adopt', '--kind', 'fresh_bootstrap'), ('run',), ('run',)]:
                subprocess.run(['python3', 'scripts/migration_runner.py', *arguments],
                    cwd=journey.ROOT, env=env, check=True)
        else:
            subprocess.run([str(journey.INSPECTOR), str(state), 'seed-combat'], check=True)
        source = FIXTURE.replace('std::vector<uint8_t> payload, bytes;',
            'snapshot.skills.push_back({1308,100,100});\n'
            'snapshot.skills.push_back({1148,100,100});\n'
            'std::vector<uint8_t> payload, bytes;')
        cpp = root / 'staff.cpp'
        cpp.write_text(source)
        fixture = root / 'staff'
        subprocess.run(['g++', '-std=c++20', '-Isrc', str(cpp),
            'src/player/player_snapshot_codec.c', 'src/flatfile/flatfile_player_snapshot_file.c',
            'src/flatfile/flatfile_store.c', '-lcrypto', '-o', str(fixture)],
            cwd=journey.ROOT, check=True)
        client = process = output = None

        def stop():
            nonlocal client, process, output
            if client:
                client.close()
                client = None
            if process and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)
            if output:
                output.close()

        def boot():
            nonlocal process, output
            output = (runtime / 'server.out').open('w')
            process = subprocess.Popen([str(runtime / 'bin/server/dms'), '--minimal', '-s', str(port)],
                cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 90
            while 'Entering game loop.' not in (runtime / 'server.out').read_text(errors='replace'):
                assert process.poll() is None and time.monotonic() < deadline, 'boot failed'
                time.sleep(.1)

        def command(text, duration=.5):
            # Staff fixture grants are asynchronous. Settle the preceding save
            # before starting another setup grant, then await its checkpoint.
            if text.startswith('load obj '):
                save_items()
            drain(client, .1)
            client.send(text)
            result = ''
            if text.startswith('stat mob '):
                result = client.expect('Events:', timeout=30)
            elif text.startswith('load obj '):
                result = client.expect('You have created', timeout=30)
            result += drain(client, duration)
            print(text + ': ' + result[-1800:], flush=True)
            if '[Return to continue' in result:
                client.send('q')
                drain(client)
            if text.startswith('load obj '):
                save_items()
            return result

        def save_items():
            client.send('save')
            client.expect('Save complete for Taverek.', timeout=30)
            if mode == 'file':
                return journey.inspect_authority(state)['player_items']
            rows = sql("SELECT own.item_uid,own.vnum,own.root_item_uid,COALESCE(own.parent_item_uid,0) "
                "FROM item_current_owner own JOIN player_data player ON player.pid=own.owner_id "
                "WHERE own.owner_type=1 AND own.state=1 AND player.name='Taverek' ORDER BY own.item_uid")
            return [dict(zip(('uid', 'vnum', 'root', 'parent'), map(int, row.split('\t'))))
                    for row in rows.splitlines()]

        def full_logs():
            return '\n'.join(path.read_text(errors='replace')
                             for path in (runtime / 'logs/log').glob('*') if path.is_file())

        try:
            boot()
            client = journey.MudClient(port)
            journey.create_character(client)
            save_items()
            client.send('quit')
            client.expect('ACCOUNT MENU', timeout=30)
            stop()
            if mode == 'file':
                subprocess.run([str(fixture), str(state)], check=True)
            else:
                sql("UPDATE player_data SET level=62,highest_level=62,base_hit=200000 WHERE name='Taverek'")
                for skill_id in (1308, 1148):
                    sql("INSERT INTO player_skills(pid,skill_id,learned,taught) "
                        f"SELECT pid,{skill_id},100,100 FROM player_data WHERE name='Taverek' "
                        "ON DUPLICATE KEY UPDATE learned=100,taught=100")
                env.update(REDIS='TRUE', REDIS_HOST='127.0.0.1', REDIS_PORT=str(redis_port),
                    REDIS_NAMESPACE='duris:local:alchemist-' + database.split('_')[-1],
                    REDIS_WORLD_STATE='TRUE',
                    REDIS_WORLD_STATE_INTERVAL='5',
                    REDIS_WORLD_STATE_SECRET='local-alchemist-fixture-secret-123456789')
            boot()
            client = journey.reconnect_character(port)
            command('toggle paging')
            granted = []
            for index in range(200):
                stat = command(f'stat mob alc{index:02d}', .15)
                match = re.search(r'Carried Items:\s*(\d+)', stat)
                assert match, stat
                if int(match.group(1)):
                    granted.append(f'alc{index:02d}')
                if len(granted) >= 3:
                    break
            assert len(granted) >= 3, 'fewer than three grants among 200 fresh spawns'
            print('Selected automatically granted NPCs: ' + ', '.join(granted), flush=True)
            signatures = {name: re.search(r'Numbers:.*?(\d+)-I', command('stat mob ' + name)).group(1)
                          for name in granted}
            for cycle in range(2):
                client.send('save')
                client.expect('Save complete for Taverek.', timeout=30)
                client.send('shutdown copyover')
                client.expect('Copyover complete!', timeout=90)
                for name in granted:
                    stat = command('stat mob ' + name)
                    assert re.search(r'Numbers:.*?(\d+)-I', stat).group(1) == signatures[name], stat
                    assert re.search(r'Carried Items:\s*(\d+)', stat).group(1) == '1', stat
                print(f'PASS copyover {cycle + 1}: selected NPC identities and one vial retained', flush=True)
            text = command('steal vial ' + granted[0], 1)
            assert 'Got it!' in text, text
            before = save_items()
            assert sum(item['vnum'] == 102 for item in before) == 1, before
            client.send('shutdown copyover')
            client.expect('Copyover complete!', timeout=90)
            assert {(item['uid'], item['vnum']) for item in before} == {
                (item['uid'], item['vnum']) for item in save_items()}
            print('PASS original socket copyover retains the stolen vial and exact player item UIDs', flush=True)
            command('restore Taverek', 1)
            command('load obj 806')
            command('load obj 808')
            inputs = save_items()
            consumed = {item['uid'] for item in inputs if item['vnum'] in (102, 806, 808)}
            assert len(consumed) == 3, inputs
            client.send('mixpoison')
            client.expect('You finish mixing 1 poison.', timeout=30)
            items = save_items()
            assert consumed.isdisjoint({item['uid'] for item in items}), items
            assert sum(item['vnum'] == 470 for item in items) == 1, items
            print('PASS stolen automatic vial -> real Assassin poison craft and exact input retirement', flush=True)
            command('kill ' + granted[1], 3)
            client.send('get all corpse')
            client.expect('vial', timeout=30)
            items = save_items()
            assert sum(item['vnum'] == 102 for item in items) == 1, items
            print('PASS real NPC death and corpse loot of automatic vial', flush=True)
            # A durable NPC is only used to hold the real combat open for observation.
            for name in (granted[2], granted[0]):
                command('setbit char ' + name + ' basehit 30000', 1)
                command('setbit char ' + name + ' hit 30000', 1)
                stat = command('stat mob ' + name)
                assert int(re.search(r'Hits:\s*\[\s*(\d+)', stat).group(1)) >= 1000, stat
            command('setattr ' + granted[2] + ' agi 103')
            client.send('force ' + granted[2] + ' kill ' + granted[0])
            client.expect('alchemical mixture', timeout=45)
            command('purge ' + granted[2])
            print('PASS real server combat scheduler invokes virtual alchemist ability', flush=True)
            for attempt in range(5):
                previous = {item['uid'] for item in save_items()}
                command('load obj 677')
                command('load obj 400291')
                inputs = save_items()
                consumed = {item['uid'] for item in inputs if item['uid'] not in previous}
                assert len(consumed) == 2, inputs
                client.send('encrust mace green')
                result, text = client.expect_any(('Hurrah! Hurrah!', 'You broke your item in the process.'), 30)
                items = save_items()
                assert consumed.isdisjoint({item['uid'] for item in items}), items
                if result == 'Hurrah! Hurrah!':
                    break
            else:
                raise AssertionError('five valid Encrust attempts failed')
            items = save_items()
            assert any(item['vnum'] == 1251 for item in items), items
            print('PASS real epic Encrust replacement', flush=True)
            for _ in range(3):
                command('load obj 400230')
            inputs = save_items()
            consumed = {item['uid'] for item in inputs if item['vnum'] == 400230}
            client.send('buy 1')
            client.expect('The Harvester accepts the soul shards and gives you a greater orb.', timeout=30)
            final = save_items()
            assert consumed.isdisjoint({item['uid'] for item in final}), final
            assert sum(item['vnum'] == 400231 for item in final) == 1, final
            expected = {(item['uid'], item['vnum']) for item in final if item['vnum'] in (470, 1251, 400231, 102)}
            rich_before = None
            if mode == 'redis':
                rich_before = sql("SELECT pi.obj_uid,SHA2(runtime.payload,256) FROM player_items pi "
                    "JOIN player_item_runtime_state runtime ON runtime.item_id=pi.id "
                    "JOIN player_data player ON player.pid=pi.pid WHERE player.name='Taverek' "
                    "AND pi.vnum IN(470,1251,400231) ORDER BY pi.obj_uid")
                assert len(rich_before.splitlines()) == 3, rich_before
                acknowledged = full_logs().count('generation and floor handoff acknowledged')
                deadline = time.monotonic() + 60
                while full_logs().count('generation and floor handoff acknowledged') <= acknowledged:
                    assert time.monotonic() < deadline, 'no fresh acknowledged Redis world snapshot'
                    time.sleep(.2)
            client.send('quit')
            client.expect('ACCOUNT MENU', timeout=30)
            stop()
            boot()
            client = journey.reconnect_character(port)
            reloaded = save_items()
            assert expected == {(item['uid'], item['vnum']) for item in reloaded
                                if item['vnum'] in (470, 1251, 400231, 102)}, reloaded
            if mode == 'redis':
                assert 'restored world recovery generation' in full_logs()
                rich_after = sql("SELECT pi.obj_uid,SHA2(runtime.payload,256) FROM player_items pi "
                    "JOIN player_item_runtime_state runtime ON runtime.item_id=pi.id "
                    "JOIN player_data player ON player.pid=pi.pid WHERE player.name='Taverek' "
                    "AND pi.vnum IN(470,1251,400231) ORDER BY pi.obj_uid")
                assert rich_before == rich_after, (rich_before, rich_after)
                for name in granted[:1]:
                    stat = command('stat mob ' + name)
                    assert re.search(r'Numbers:.*?(\d+)-I', stat).group(1) == signatures[name], stat
                    assert re.search(r'Carried Items:\s*(\d+)', stat).group(1) == '0', stat
                print('PASS SQL rich-state bytes and depleted NPC identity survive Redis cold recovery without reroll', flush=True)
            print('PASS Harvester exact retirement and all craft/vial UIDs survive cold player reload', flush=True)
        except Exception:
            print((runtime / 'server.out').read_text(errors='replace')[-6000:])
            print(journey.runtime_logs(runtime)[-12000:])
            print('\n'.join(line for line in full_logs().splitlines()
                            if 'PLAYER SAVE TRACE: stage=completion' in line)[-12000:])
            if client:
                print(client.transcript.decode(errors='replace')[-9000:])
            raise
        finally:
            stop()
            if database:
                sql('DROP DATABASE ' + database, False)


if __name__ == '__main__':
    run(Path(sys.argv[1]).resolve(), sys.argv[2] if len(sys.argv) > 2 else 'file')
