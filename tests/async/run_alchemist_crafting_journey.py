#!/usr/bin/env python3
"""Real server alchemist loot, supported crafts, copyover and player reload.

Uses the existing isolated combat journey's account, socket and world helpers.
Requires the combined #551/#661 server and the flat-file repository inspector.
The optional --recipe-only mode isolates mortal Craft/Forge progression,
retained pouch counters, copyover and cold restarts.
Adding --material-downgrade verifies a two-output salvage downgrade through the
same copyover and cold restarts, with exact original and output UIDs.
The optional --creation-save-only mode qualifies #664 with overlapping setup
grants and saves, then copyover, disconnect and cold reload, without crafting.
"""
from pathlib import Path
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey
from run_generated_npc_journey import FIXTURE, drain


def authored(vnum):
    for path in sorted((journey.ROOT / 'areas/obj').glob('*.obj')):
        match = re.search(rf'(?ms)^#{vnum}\n.*?(?=^#\d+\n|^\$~|\Z)', path.read_text())
        if match:
            return match.group(0)
    raise AssertionError(f'missing authored object {vnum}')


def run(binary, mode='file', creation_save_only=False, recipe_only=False, material_downgrade=False):
    assert mode in ('file', 'redis')
    assert not (creation_save_only and recipe_only), 'select one focused journey'
    assert not material_downgrade or recipe_only, 'material downgrade extends the recipe journey'
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
        recipe = re.search(r'(?ms)^#678\n.*?(?=^#\d+\n)', objects).group(0)
        recipe = recipe.replace('#678\n', '#30101\n', 1).replace('cap brown brownish leather green stitching~', 'recipe cap testcraft~')
        objects = objects.replace('$~', recipe + '$~')
        for vnum in (102, 470, 806, 808, 1251, 868, 866, 865, 863, 859,
                     857, 855, 853, 850, 400230, 400231, 400291, 400300, 400045, 400049, 400211, 400223, 400224):
            if not re.search(rf'(?m)^#{vnum}$', objects):
                objects = objects.replace('$~', authored(vnum) + '$~')
        if material_downgrade:
            for vnum in (400046, 400047):
                block = authored(vnum)
                if vnum == 400047:
                    lines = block.splitlines(keepends=True)
                    lines[1] = 'downgradeprobe material leather~\n'
                    block = ''.join(lines)
                assert not re.search(rf'(?m)^#{vnum}$', objects)
                objects = objects.replace('$~', block + '$~')
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
            'snapshot.skills.push_back({1132,100,100});\n'
            'snapshot.skills.push_back({1134,100,100});\n'
            'snapshot.skills.push_back({1297,100,100});\n'
            'auto established=flatfile_recipe_establish(root,{},&error);\n'
            'assert(established==flatfile_recipe_result::ok || established==flatfile_recipe_result::already_exists);\n'
            'auto recipe_result=flatfile_recipe_add(root,pid,30101,&error);\n'
            'assert(recipe_result==flatfile_recipe_result::ok || recipe_result==flatfile_recipe_result::unchanged);\n'
            'std::vector<uint8_t> payload, bytes;')
        source = '#include "flatfile/flatfile_recipe_repository.h"\n#include <iostream>\n' + source
        source = source.replace('assert(argc == 2);', 'assert(argc == 2 || argc == 3);')
        source = source.replace('for (auto &field : snapshot.status_integers) {',
            'if (argc==3) { for (auto &field:snapshot.status_integers) { '
            'if (field.field==player_status_field::experience && std::string(argv[2])=="inspect") { '
            'std::cout << field.signed_value << std::endl; return 0; } '
            'if (field.field==player_status_field::level || field.field==player_status_field::highest_level) '
            'field.signed_value=field.unsigned_value=50; } '
            'std::vector<uint8_t> bytes; assert(flatfile_player_snapshot_encode_file(snapshot,&bytes)); '
            'assert(flatfile_atomic_write(flatfile_player_snapshot_file::player_directory(root),'
            'flatfile_player_snapshot_file::player_filename(pid),bytes,&error)); return 0; } '
            'for (auto &field : snapshot.status_integers) {')
        cpp = root / 'staff.cpp'
        cpp.write_text(source)
        fixture = root / 'staff'
        subprocess.run(['g++', '-std=c++20', '-Isrc', str(cpp),
            'src/player/player_snapshot_codec.c', 'src/flatfile/flatfile_player_snapshot_file.c',
            'src/flatfile/flatfile_store.c', 'src/flatfile/flatfile_recipe_repository.c',
            'src/flatfile/flatfile_authority_transaction.c', '-lcrypto', '-pthread', '-o', str(fixture)],
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
            if text.startswith('load obj ') and not creation_save_only:
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
            if text.startswith('load obj ') and not creation_save_only:
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
                for skill_id in (1308, 1148, 1132, 1134, 1297):
                    sql("INSERT INTO player_skills(pid,skill_id,learned,taught) "
                        f"SELECT pid,{skill_id},100,100 FROM player_data WHERE name='Taverek' "
                        "ON DUPLICATE KEY UPDATE learned=100,taught=100")
                sql("INSERT INTO player_recipes(pid,recipe_vnum) SELECT pid,30101 FROM player_data WHERE name='Taverek'")
                env.update(REDIS='TRUE', REDIS_HOST='127.0.0.1', REDIS_PORT=str(redis_port),
                    REDIS_NAMESPACE='duris:local:alchemist-' + database.split('_')[-1],
                    REDIS_WORLD_STATE='TRUE',
                    REDIS_WORLD_STATE_INTERVAL='5',
                    REDIS_WORLD_STATE_SECRET='local-alchemist-fixture-secret-123456789')
            boot()
            client = journey.reconnect_character(port)
            command('toggle paging')
            if not recipe_only:
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
                if creation_save_only:
                    original = {(item['uid'], item['vnum']) for item in save_items()}
                    for cycle in range(8):
                        # Await only the existing creation response, as in #664;
                        # no checkpoint barrier between the two setup grants.
                        # A simultaneous second load is intentionally refused by
                        # the existing command busy gate, so submit it after the
                        # first grant's publication while its save may still run.
                        if cycle % 2:
                            first = command('load obj 806\nsave')
                        else:
                            client.send('save')
                            first = command('load obj 806')
                        second = command('load obj 808')
                        assert 'Save attempt failed' not in first + second, first + second
                        items = save_items()
                        assert len({item['uid'] for item in items}) == len(items), items
                        assert sum(item['vnum'] == 806 for item in items) == cycle + 1, items
                        assert sum(item['vnum'] == 808 for item in items) == cycle + 1, items
                    expected = {(item['uid'], item['vnum']) for item in items}
                    assert original <= expected and len(expected - original) == 16
                    client.send('shutdown copyover')
                    client.expect('Copyover complete!', timeout=90)
                    assert expected == {(item['uid'], item['vnum']) for item in save_items()}
                    client.send('quit')
                    client.expect('ACCOUNT MENU', timeout=30)
                    stop()
                    boot()
                    client = journey.reconnect_character(port)
                    assert expected == {(item['uid'], item['vnum']) for item in save_items()}
                    logs = full_logs()
                    assert 'active_custody_absent_from_snapshot' not in logs
                    assert 'outcome=terminal_failure' not in logs
                    print('PASS eight overlapping save/grant rounds: 16 exact new UIDs, copyover, disconnect and cold reload', flush=True)
                    return
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
                # Enable the real pouch feature only after the ordinary starter and
                # physical-craft journey, so the original fixture remains intact.
                client.send('quit')
                client.expect('ACCOUNT MENU', timeout=30)
                stop()
                env.update(CHAOS_MUD='TRUE', CHAOS_STARTER_FRIGATE='FALSE')
                boot()
                client = journey.reconnect_character(port)
                command('load obj 400300')
                pouch_items = [item for item in save_items() if item['vnum'] == 400300]
                assert len(pouch_items) == 1, pouch_items
                pouch_uid = pouch_items[0]['uid']
                for _ in range(2):
                    command('load obj 400291')
                collected = {item['uid'] for item in save_items() if item['vnum'] == 400291}
                assert len(collected) == 2, collected
                client.send('put all.green pouch')
                client.expect('You record 2 collected materials', timeout=30)
                assert collected.isdisjoint({item['uid'] for item in save_items()})
                attempts = 0
                for attempts in range(1, 6):
                    previous = {item['uid'] for item in save_items()}
                    command('load obj 677')
                    bases = {item['uid'] for item in save_items()
                             if item['vnum'] == 677 and item['uid'] not in previous}
                    assert len(bases) == 1, bases
                    # The preceding physical journey leaves an already-encrusted
                    # mace in inventory. Target the newly loaded second match.
                    client.send('encrust 2.mace 400291')
                    result, _ = client.expect_any(('Hurrah! Hurrah!', 'You broke your item in the process.'), 30)
                    current = save_items()
                    assert bases.isdisjoint({item['uid'] for item in current}), current
                    assert not any(item['vnum'] == 400291 for item in current), current
                    assert [item['uid'] for item in current if item['vnum'] == 400300] == [pouch_uid]
                    if result == 'Hurrah! Hurrah!':
                        break
                else:
                    raise AssertionError('five valid virtual Encrust attempts failed')

                def pouch_scores():
                    text = command('look in pouch')
                    clean = re.sub(r'\x1b\[[0-9;]*[A-Za-z]', '', text)
                    assert re.search(rf'\b{attempts} generated / 2 collected\b', clean), clean
                    assert '[400291]' in clean, clean
                    return clean

                pouch_scores()
                client.send('shutdown copyover')
                client.expect('Copyover complete!', timeout=90)
                pouch_scores()
                expected_pouch_items = {(item['uid'], item['vnum']) for item in save_items()}
                client.send('quit')
                client.expect('ACCOUNT MENU', timeout=30)
                stop()
                boot()
                client = journey.reconnect_character(port)
                assert expected_pouch_items == {(item['uid'], item['vnum']) for item in save_items()}
                pouch_scores()
                print('PASS real pouch collection and virtual Encrust preserve counters and the original pouch UID through copyover and cold reload', flush=True)
            else:
                command('load obj 400300')
                pouch_items = [item for item in save_items() if item['vnum'] == 400300]
                assert len(pouch_items) == 1, pouch_items
                pouch_uid = pouch_items[0]['uid']

            # Freeze one authored leather recipe and grant setup requirements
            # before switching this fixture to a mortal, so real XP can change.
            information = re.sub(r'\x1b\[[0-9;]*[A-Za-z]', '', command('craft info 30101', 1))
            high = int(re.search(r'need (\d+) of', information).group(1))
            low_match = re.search(r'and (\d+) of', information)
            low = int(low_match.group(1)) if low_match else 0
            assert 1 <= high <= 10 and 0 <= low <= 4, information
            for vnum, count in ((400049, 2 * high), (400045, 2 * low), (400224, 2), (400223, 2)):
                for _ in range(count):
                    command(f'load obj {vnum}')
            if material_downgrade:
                command('load obj 400047')
            seeded = save_items()
            material_uids = {item['uid'] for item in seeded if item['vnum'] in (400045, 400049)}
            tool_uids = {item['uid'] for item in seeded if item['vnum'] in (400223, 400224)}
            assert len(material_uids) == 2 * (high + low) and len(tool_uids) == 4
            client.send('quit')
            client.expect('ACCOUNT MENU', timeout=30)
            stop()
            if mode == 'file':
                subprocess.run([str(fixture), str(state), 'mortal'], check=True)
            else:
                sql("UPDATE player_data SET level=50,highest_level=50 WHERE name='Taverek'")
            env['CHAOS_MUD'] = 'FALSE'
            boot()
            client = journey.reconnect_character(port)

            def durable_recipe_experience():
                save_items()
                if mode == 'file':
                    return int(subprocess.check_output([str(fixture), str(state), 'inspect'], text=True))
                return int(sql("SELECT exp FROM player_data WHERE name='Taverek'"))

            before_experience = durable_recipe_experience()
            for discipline in ('craft', 'forge'):
                before = save_items()
                before_uids = {item['uid'] for item in before}
                client.send(f'{discipline} make 30101')
                client.expect('finish your work, admiring your new', timeout=30)
                after = save_items()
                fresh = [item for item in after if item['uid'] not in before_uids]
                assert len(fresh) == 1 and fresh[0]['vnum'] == 30101, (discipline, fresh)
                retired = [item for item in before if item['uid'] not in {row['uid'] for row in after}]
                assert len(retired) == high + low + 1, (discipline, retired)
                assert sum(item['vnum'] == 400049 for item in retired) == high
                assert sum(item['vnum'] == 400045 for item in retired) == low
                assert sum(item['vnum'] == (400224 if discipline == 'craft' else 400223) for item in retired) == 1
                experience = durable_recipe_experience()
                assert experience > before_experience, (discipline, before_experience, experience)
                before_experience = experience
            assert material_uids.isdisjoint({item['uid'] for item in save_items()})
            print('PASS real mortal Craft and Forge retire exact materials/tools, admit fresh outputs and save XP', flush=True)
            client.send('quit')
            client.expect('ACCOUNT MENU', timeout=30)
            stop()
            env['CHAOS_MUD'] = 'TRUE'
            boot()
            client = journey.reconnect_character(port)
            for discipline in ('craft', 'forge'):
                before = save_items()
                before_uids = {item['uid'] for item in before}
                client.send(f'{discipline} make 30101')
                client.expect('finish your work, admiring your new', timeout=30)
                after = save_items()
                retired = [item for item in before if item['uid'] not in {row['uid'] for row in after}]
                fresh = [item for item in after if item['uid'] not in before_uids]
                assert len(retired) == 1 and retired[0]['vnum'] == (400224 if discipline == 'craft' else 400223)
                assert len(fresh) == 1 and fresh[0]['vnum'] == 30101
                assert [item['uid'] for item in after if item['vnum'] == 400300] == [pouch_uid]
                experience = durable_recipe_experience()
                assert experience > before_experience
                before_experience = experience
            assert tool_uids.isdisjoint({item['uid'] for item in save_items()})
            recipe_scores = command('look in pouch', 1)
            assert f'{2 * high} generated / 0 collected' in recipe_scores, recipe_scores
            if material_downgrade:
                before = save_items()
                original = [item for item in before if item['vnum'] == 400047]
                assert len(original) == 1, original
                client.send('salvage downgradeprobe')
                client.expect('You break down your material into two lesser materials.', timeout=30)
                after = save_items()
                before_uids = {item['uid'] for item in before}
                after_uids = {item['uid'] for item in after}
                fresh = [item for item in after if item['uid'] not in before_uids]
                assert before_uids - after_uids == {original[0]['uid']}
                assert len(fresh) == 2 and all(item['vnum'] == 400046 for item in fresh), fresh
                assert len({item['uid'] for item in fresh}) == 2
                if mode != 'file':
                    assert sql(f"SELECT state FROM item_current_owner WHERE item_uid={original[0]['uid']}") == '2'
                client.send('salvage downgradeprobe')
                client.expect('What would you like to salvage?', timeout=30)
                assert {(item['uid'], item['vnum']) for item in save_items()} == {
                    (item['uid'], item['vnum']) for item in after}
                print('PASS material downgrade retires original once and admits exactly two fresh lower-tier UIDs', flush=True)
            expected_items = {(item['uid'], item['vnum']) for item in save_items()}
            expected_experience = durable_recipe_experience()
            # Exercise the normal launcher lifecycle request while the actor
            # remains mortal and eligible for progression awards.
            process.send_signal(signal.SIGUSR1)
            client.expect('Copyover complete!', timeout=90)
            assert durable_recipe_experience() == expected_experience
            for _ in range(2):
                client.send('quit')
                client.expect('ACCOUNT MENU', timeout=30)
                stop()
                boot()
                client = journey.reconnect_character(port)
                assert expected_items == {(item['uid'], item['vnum']) for item in save_items()}
                assert durable_recipe_experience() == expected_experience
                assert f'{2 * high} generated / 0 collected' in command('look in pouch', 1)
            print('PASS retained pouch Craft/Forge output UIDs, counters and exact XP survive copyover and two cold restarts', flush=True)
            if material_downgrade:
                print('PASS material downgrade exact output UIDs survive copyover and two cold restarts', flush=True)
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
    run(Path(sys.argv[1]).resolve(), sys.argv[2] if len(sys.argv) > 2 else 'file',
        '--creation-save-only' in sys.argv[3:], '--recipe-only' in sys.argv[3:],
        '--material-downgrade' in sys.argv[3:])
