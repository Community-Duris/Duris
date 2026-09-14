#!/usr/bin/env python3
"""Two real Telnet players repeatedly engage, purge the opponent, and fully reset.

Usage: python3 tests/async/run_zone_purge_journey.py /absolute/flatfile/server
The private minimal world is separate from the reported full-world room 402003.
"""
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
    assert(argc == 2);
    const std::string root = argv[1];
    for (uint32_t pid : {1, 2}) {
        player_snapshot snapshot; std::string error;
        assert(flatfile_player_snapshot_read(root,pid,&snapshot,&error) == flatfile_player_load_result::ok);
        for (auto &field : snapshot.status_integers) {
            if (field.field == player_status_field::level || field.field == player_status_field::highest_level)
                field.signed_value = field.unsigned_value = pid == 1 ? 60 : 56;
            if (field.field == player_status_field::base_hit) field.signed_value = field.unsigned_value = 200000;
            if (field.field == player_status_field::hit_difference) field.signed_value = field.unsigned_value = 0;
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


def run(binary):
    subprocess.run(['python3', 'tests/async/test_flatfile_player_repository.py',
                    '--build-inspector', str(journey.INSPECTOR)], cwd=journey.ROOT, check=True)
    with tempfile.TemporaryDirectory(prefix='duris-purge-journey-') as temporary:
        root = Path(temporary)
        state, runtime = root / 'state', root / 'runtime'
        state.mkdir(mode=0o700); (state / 'domains').mkdir(mode=0o700)
        runtime.mkdir(); (runtime / 'logs/log').mkdir(parents=True)
        (runtime / 'logs/log/.gitignore').write_text('*\n')
        journey.make_fixture(runtime)
        zone = runtime / 'areas_mini/mini.zon'
        zone.write_text(re.sub(r'^M 0 22800 .*\n', '', zone.read_text(), flags=re.M))
        properties = runtime / 'lib/duris.properties'
        properties.write_text(re.sub(r'(?m)^hitpoints.mob.NpcPcRatio=.*$',
                                     'hitpoints.mob.NpcPcRatio=1000', properties.read_text()))
        # Keep normal weak weapon damage while the NPC survives each engage.
        objects = runtime / 'areas_mini/mini.obj'
        objects.write_text(objects.read_text().replace('6 100 1 7 0 0 0 0', '6 1 6 7 0 0 0 0'))
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

        def reconnect_player():
            original = journey.ACCOUNT, journey.CHARACTER
            journey.ACCOUNT, journey.CHARACTER = 'Purgeacct', 'Purgemortal'
            try: return journey.reconnect_character(port)
            finally: journey.ACCOUNT, journey.CHARACTER = original

        def stat_player():
            admin.send('stat c Purgemortal')
            status = drain(admin, 0.8)
            if '[Return to continue' in status:
                admin.send('q')
                drain(admin)
            return status

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
            subprocess.run([str(fixture), str(state)], check=True)
            boot(); admin = journey.reconnect_character(port); player = reconnect_player()
            drain(admin); drain(player)
            for cycle in range(15):
                admin.send('zreset full'); admin.expect('has been reset.')
                drain(admin); drain(player)
                for attempt in range(6):
                    player.send('kill raoul'); drain(player)
                    status = stat_player()
                    if re.search(r'Fighting:\s+Raoul', status, re.I): break
                else: raise AssertionError('mortal did not engage Raoul: ' + status[-3000:])
                admin.send('purge raoul'); drain(admin); drain(player)
                status = stat_player()
                assert re.search(r'Fighting:\s+---', status), status[-3000:]
                admin.send('zreset full'); admin.expect('has been reset.')
                player.send('look'); player.expect('The Regression Arena')
                assert process.poll() is None
                print(f'cycle {cycle + 1}: verified live combat, purged opponent, cleared combat reference, full reset', flush=True)
            player.send('save'); player.expect('Save complete for Purgemortal.', timeout=30)
            stop(); boot(); player = reconnect_player()
            player.send('look'); player.expect('The Regression Arena')
            print('two-player purge/reset, save and restart journey passed', flush=True)
        except Exception:
            print((runtime / 'server.out').read_text(errors='replace')[-6000:])
            for client in (admin, player):
                if client: print(client.transcript.decode(errors='replace')[-8000:])
            raise
        finally: stop()


if __name__ == '__main__':
    run(Path(sys.argv[1]).resolve())
