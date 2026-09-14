#!/usr/bin/env python3
"""A staff fixture casts a floor; a real player survives or dies on impact.

Usage: python3 tests/async/run_lethal_floor_journey.py /absolute/server lethal|nonlethal
Uses only synthetic accounts, offline fixture snapshots and a private mini world.
"""
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
import test_flatfile_combat_journey as journey

FIXTURE = r'''
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"
#include <cassert>
#include <openssl/sha.h>
template<class T> void number(std::vector<uint8_t>& out, T value) {
    for (size_t i=0; i<sizeof(T); ++i) out.push_back(static_cast<uint64_t>(value) >> (i*8));
}
int main(int argc, char **argv) {
    assert(argc == 3);
    const std::string root = argv[1];
    for (uint32_t pid : {1, 2}) {
        player_snapshot snapshot; std::string error;
        assert(flatfile_player_snapshot_read(root,pid,&snapshot,&error) == flatfile_player_load_result::ok);
        for (auto &field : snapshot.status_integers) {
            if (field.field == player_status_field::level || field.field == player_status_field::highest_level)
                field.signed_value = field.unsigned_value = pid == 1 ? 62 : 1;
            if (field.field == player_status_field::base_hit) field.signed_value = field.unsigned_value = 200000;
            if (field.field == player_status_field::hit_difference) field.signed_value = field.unsigned_value = (pid == 2 && std::string(argv[2]) == "lethal") ? 199000 : 0;
        }
        std::vector<uint8_t> payload, bytes;
        snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
        assert(player_snapshot_encode(snapshot,&payload) == player_snapshot_codec_result::ok);
        using namespace flatfile_player_snapshot_file;
        bytes.insert(bytes.end(),player_magic.begin(),player_magic.end());
        number(bytes,player_file_version); number<uint32_t>(bytes,payload.size());
        number(bytes,snapshot.pid); number(bytes,snapshot.revision); number(bytes,snapshot.components);
        unsigned char digest[SHA256_DIGEST_LENGTH]; SHA256(payload.data(),payload.size(),digest);
        bytes.insert(bytes.end(),digest,digest+sizeof(digest)); bytes.insert(bytes.end(),payload.begin(),payload.end());
        assert(flatfile_atomic_write(player_directory(root),player_filename(pid),bytes,&error));
    }
}
'''


def drain(client, duration=0.5):
    until = time.monotonic() + duration
    while time.monotonic() < until:
        client._receive()
    text = client.pending.decode(errors='replace')
    client.pending.clear()
    return text


def run(binary, mode):
    subprocess.run(['python3', 'tests/async/test_flatfile_player_repository.py',
                    '--build-inspector', str(journey.INSPECTOR)], cwd=journey.ROOT, check=True)
    assert mode in ('lethal', 'nonlethal')
    with tempfile.TemporaryDirectory(prefix='duris-floor-journey-') as temporary:
        root = Path(temporary)
        state, runtime = root / 'state', root / 'runtime'
        state.mkdir(mode=0o700); (state / 'domains').mkdir(mode=0o700)
        runtime.mkdir(); (runtime / 'logs/log').mkdir(parents=True)
        (runtime / 'logs/log/.gitignore').write_text('*\n')
        journey.make_fixture(runtime)
        zone = runtime / 'areas_mini/mini.zon'
        zone.write_text(re.sub(r'^M 0 22800 .*\n', '', zone.read_text(), flags=re.M))
        zone.write_text(re.sub(r'^[MG] .*\n', '', zone.read_text(), flags=re.M))
        world = runtime / 'areas_mini/mini.wld'
        text = world.read_text().replace('1 0 0\nS\n$~', '1 0 0\nD1\n~\n~\n0 0 22801\nS\n$~')
        rooms = '''#22801
The Upper Ledge~
An open shaft descends.\n~
1 0 8
D5
~
~
0 0 22802
S
#22802
The Lower Ledge~
The shaft continues.\n~
1 0 8
D5
~
~
0 0 22803
S
#22803
The Last Ledge~
The shaft continues.\n~
1 0 8
D5
~
~
0 0 22804
S
#22804
The Breakable Floor~
A spell-created floor can interrupt a falling character.\n~
1 0 8
D5
~
~
0 0 22805
S
#22805
The Final Landing~
A solid stone floor stops the fall.\n~
1 0 0
D4
~
~
0 0 22804
S
'''
        assert '0 0 22801' in text
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
        source = root / 'fixture.cpp'; source.write_text(FIXTURE)
        fixture = root / 'fixture'
        subprocess.run(['g++', '-std=c++20', '-Isrc', str(source),
                        'src/player/player_snapshot_codec.c', 'src/flatfile/flatfile_player_snapshot_file.c',
                        'src/flatfile/flatfile_store.c', '-lcrypto', '-o', str(fixture)], cwd=journey.ROOT, check=True)
        process = output = None
        admin = player = None

        def stop():
            nonlocal process, output, admin, player
            for client in (admin, player):
                if client: client.close()
            admin = player = None
            if process and process.poll() is None:
                process.terminate()
                try: process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait(timeout=5); raise
            if output: output.close()

        def boot():
            nonlocal process, output
            output = (runtime / 'server.out').open('w')
            process = subprocess.Popen([str(binary), '--minimal', '-s', '-d', str(runtime), str(port)],
                                       cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 90
            while 'Entering game loop.' not in (runtime / 'server.out').read_text(errors='replace'):
                assert process.poll() is None and time.monotonic() < deadline, 'boot failed'
                time.sleep(0.1)

        def reconnect_player(expected_room='The Regression Arena'):
            original = journey.ACCOUNT, journey.CHARACTER
            journey.ACCOUNT, journey.CHARACTER = 'Purgeacct', 'Purgemortal'
            try: return journey.reconnect_character(port, expected_room=expected_room)
            finally: journey.ACCOUNT, journey.CHARACTER = original

        def inspect_player():
            return json.loads(subprocess.check_output(
                [str(journey.INSPECTOR), str(state), 'inspect', '2'], text=True))

        try:
            boot()
            admin = journey.MudClient(port); journey.create_character(admin)
            admin.send('save'); admin.expect(f'Save complete for {journey.CHARACTER}.')
            admin.send('quit'); admin.expect('ACCOUNT MENU', timeout=30)
            player = journey.MudClient(port)
            journey.create_character(player, account='Purgeacct', character='Purgemortal', email='purge@example.invalid')
            player.send('save'); player.expect('Save complete for Purgemortal.')
            player.send('quit'); player.expect('ACCOUNT MENU', timeout=30)
            stop()
            subprocess.run([str(fixture), str(state), mode], check=True)
            boot(); admin = journey.reconnect_character(port); player = reconnect_player()
            drain(admin); drain(player)
            admin.send('goto 22804'); admin.expect('The Breakable Floor')
            admin.send("cast 'wall of ice' down")
            admin.expect('a huge block of ice forms', timeout=30)
            drain(admin); drain(player)
            before_death = inspect_player()
            assert not (state / 'domains/world_item_catalog').exists(), 'fixture hid first-corpse initialization'
            player.send('east'); player.expect('You rediscover the law of gravity')
            player.expect('You slam into', timeout=30)
            if mode == 'lethal':
                player.expect('Your spirit slips free of your body', timeout=30)
                drain(admin)
                admin.send('look'); print('floor immediately after death: ' + drain(admin, 1), flush=True)
                player.expect('ACCOUNT MENU', timeout=60)
                print('lethal player reached account menu', flush=True)
            else:
                player.expect('You land with stunning force!', timeout=30)
                time.sleep(5)
                player.send('save')
                outcome, _ = player.expect_any(('Save complete for Purgemortal.', 'Being knocked unconscious strictly limits what you can do.'), timeout=30)
                if outcome != 'Save complete for Purgemortal.':
                    player.expect('Feeling begins to return', timeout=30)
                    player.send('save'); player.expect('Save complete for Purgemortal.', timeout=30)
                player.send('look'); player.expect('The Final Landing')
            drain(admin)
            admin.send('look'); room = drain(admin, 1)
            print(mode + ' floor room: ' + room, flush=True)
            if mode == 'lethal':
                assert 'huge block of solid ice' in room.lower(), room
                assert 'corpse of a Human' in room, room
                assert 'quite dead' not in room, room
            else:
                assert 'huge block of solid ice' not in room.lower(), room
            assert process.poll() is None
            if mode == 'lethal':
                after_death = inspect_player()
                assert after_death['death_count'] == before_death['death_count'] + 1
                assert not after_death['player_items'], after_death
                original_uids = set(before_death['snapshot_uids'])
                assert original_uids, 'fixture must carry real starting equipment'
                admin.send('look in purgemortal'); contents = drain(admin, 1)
                print('corpse contents before restart: ' + contents, flush=True)
                assert 'does not seem to be here' not in contents.lower(), contents
                player.close()
                player = reconnect_player()
                admin.send('transfer purgemortal'); drain(admin); drain(player)
                player.send('get all purgemortal'); player.expect('You get', timeout=30)
                time.sleep(2)
                player.send('save'); player.expect('Save complete for Purgemortal.', timeout=30)
                recovered = inspect_player()
                recovered_uids = set(recovered['snapshot_uids'])
                # Ordinary carry limits can stop a bulk haul. Every recovered
                # item must be from the original corpse, with its identity intact.
                assert recovered_uids and recovered_uids.issubset(original_uids), recovered
                assert recovered['death_count'] == after_death['death_count']
                print(f'recovered {len(recovered_uids)} of {len(original_uids)} original items within normal carry limits', flush=True)
                # Minimal mode deliberately skips restoreCorpses at startup.
                # Recover through real commands first, then verify the saved items
                # and death count through actual process restart and re-entry.
                stop(); boot()
                player = reconnect_player(expected_room='The Breakable Floor')
                player.send('save'); player.expect('Save complete for Purgemortal.', timeout=30)
                restarted = inspect_player()
                assert restarted['snapshot_uids'] == recovered['snapshot_uids'], restarted
                assert restarted['death_count'] == recovered['death_count']
                print('lethal: re-entry, exact recovered-item identities, restart and player save passed', flush=True)
            print(mode + ': actual wall spell, fall impact, floor/corpse placement and live command checks passed', flush=True)
        except Exception:
            print((runtime / 'server.out').read_text(errors='replace')[-6000:])
            print(journey.runtime_logs(runtime)[-16000:])
            for client in (admin, player):
                if client: print(client.transcript.decode(errors='replace')[-8000:])
            raise
        finally: stop()


if __name__ == '__main__':
    run(Path(sys.argv[1]).resolve(), sys.argv[2])
