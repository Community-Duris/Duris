#!/usr/bin/env python3
"""Real Telnet: create, save, restart, walk off a ledge, land, save and reload.

Usage: python3 tests/async/run_falling_skills_journey.py /absolute/flatfile/server
Uses synthetic authority and a minimal world. The offline helper controls skill
and HP; server movement, event scheduling, damage, output and persistence are real.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
import test_flatfile_combat_journey as journey


def run(binary):
    subprocess.run(['python3', 'tests/async/test_flatfile_player_repository.py',
                    '--build-inspector', str(journey.INSPECTOR)], cwd=journey.ROOT, check=True)
    with tempfile.TemporaryDirectory(prefix='duris-falling-journey-') as temporary:
        root = Path(temporary)
        state, runtime = root / 'state', root / 'runtime'
        state.mkdir(mode=0o700)
        (state / 'domains').mkdir(mode=0o700)
        runtime.mkdir()
        (runtime / 'logs/log').mkdir(parents=True)
        (runtime / 'logs/log/.gitignore').write_text('*\n')
        journey.make_fixture(runtime)
        zone = runtime / 'areas_mini/mini.zon'
        zone.write_text(re.sub(r'^[MG] .*\n', '', zone.read_text(), flags=re.M))
        world = runtime / 'areas_mini/mini.wld'
        text = world.read_text()
        text = text.replace('1 0 0\nS\n$~', '1 0 0\nD1\n~\n~\n0 0 22801\nS\n$~')
        assert '0 0 22801' in text, 'arena exit fixture changed'
        rooms = '''#22801
The Regression Ledge~
A narrow gap leads to the landing below.\n~
1 0 8
D5
~
~
0 0 22802
S
#22802
The Regression Landing~
A stone floor stops the fall.\n~
1 0 0
S
'''
        world.write_text(text.replace('$~', rooms + '$~'))
        journey.generate_certificate(runtime)
        for name in ('players', 'critical'):
            (runtime / 'journals' / name).mkdir(parents=True, mode=0o700)
        port, tls, websocket = journey.available_ports()
        env = dict(PATH=os.environ.get('PATH', '/usr/bin:/bin'), ENVIRONMENT='local',
                   PERSISTENCE_MODE='flatfile-primary', FLATFILE_STATE_DIR=str(state),
                   PLAYER_SAVE_JOURNAL_DIR=str(runtime / 'journals/players'),
                   CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / 'journals/critical'),
                   LISTEN_ADDRESS='127.0.0.1', DURIS_TLS_PORT=str(tls),
                   DURIS_WEBSOCKET_LISTEN_ADDRESS='127.0.0.1', DURIS_WEBSOCKET_PORT=str(websocket),
                   REDIS='FALSE', CHAOS_MUD='FALSE')
        subprocess.run([str(journey.INSPECTOR), str(state), 'seed-combat'], check=True)
        fixture = root / 'fixture'
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc',
                        'tests/async/falling_journey_fixture.cpp', 'src/player/player_snapshot_codec.c',
                        'src/flatfile/flatfile_player_snapshot_file.c', 'src/flatfile/flatfile_store.c',
                        '-lcrypto', '-o', str(fixture)], cwd=journey.ROOT, check=True)
        process = client = output = None

        def stop():
            nonlocal process, client, output
            if client:
                client.close()
                client = None
            if process and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
                    raise
            if output:
                output.close()

        def boot():
            nonlocal process, output
            output = (runtime / 'server.out').open('w')
            process = subprocess.Popen([str(binary), '--minimal', '-s', '-d', str(runtime), str(port)],
                                       cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 90
            while 'Entering game loop.' not in (runtime / 'server.out').read_text(errors='replace'):
                assert process.poll() is None and time.monotonic() < deadline, 'boot failed'
                time.sleep(0.1)

        try:
            boot()
            client = journey.MudClient(port)
            journey.create_character(client)
            client.send('toggle boon')
            client.expect('You will no longer be affected by boons.')
            client.send('save')
            client.expect(f'Save complete for {journey.CHARACTER}.', timeout=30)
            client.send('quit')
            client.expect('ACCOUNT MENU', timeout=30)
            stop()
            losses = {}
            for mode in ('unskilled', 'safe', 'climb-zero'):
                for attempt in range(5 if mode == 'safe' else 1):
                    subprocess.run([str(fixture), str(state), mode], check=True)
                    boot()
                    client = journey.reconnect_character(port)
                    client.send('east')
                    client.expect('You rediscover the law of gravity', timeout=20)
                    client.expect('You land with stunning force!', timeout=20)
                    client.send('save')
                    saved_message = f'Save complete for {journey.CHARACTER}.'
                    outcome, _ = client.expect_any((saved_message,
                        'Being knocked unconscious strictly limits what you can do.'), timeout=30)
                    if outcome != saved_message:
                        client.expect('Feeling begins to return', timeout=30)
                        client.send('save')
                        client.expect(saved_message, timeout=30)
                    saved = subprocess.check_output([str(fixture), str(state), 'inspect'], text=True).split()
                    assert int(saved[0]) == 22802, saved
                    losses[mode] = int(saved[1])
                    assert losses[mode] > 1000, losses
                    print(f'{mode} attempt {attempt + 1}: room {saved[0]}, persisted HP loss {saved[1]}', flush=True)
                    stop()
                    if mode != 'safe':
                        break
                    ratio = losses['safe'] / losses['unskilled']
                    if 0.4 < ratio < 0.6:
                        break
                    assert 0.9 < ratio < 1.1, f'Safe Fall increased damage: {losses}'
                    print('Safe Fall roll failed; retrying the unchanged character setup', flush=True)
                if mode == 'safe':
                    assert 0.4 < losses['safe'] / losses['unskilled'] < 0.6, losses
                boot()
                client = journey.reconnect_character(port, expected_room='The Regression Landing')
                client.send('look')
                client.expect('The Regression Landing')
                client.send('save')
                client.expect(f'Save complete for {journey.CHARACTER}.', timeout=30)
                stop()
            # Normal RNG and a possible regeneration tick cannot mask a doubled
            # impact with this HP scale. Exact integer cases have their own test.
            assert 0.4 < losses['safe'] / losses['unskilled'] < 0.6, losses
            assert 0.9 < losses['climb-zero'] / losses['unskilled'] < 1.1, losses
            print('falling skills: real movement, landing, damage, save/restart/reload passed', flush=True)
        except Exception:
            print((runtime / 'server.out').read_text(errors='replace')[-8000:])
            if client:
                print(client.transcript.decode(errors='replace')[-8000:])
            raise
        finally:
            stop()


if __name__ == '__main__':
    run(Path(sys.argv[1]).resolve())
