#!/usr/bin/env python3
"""Real flatfile copyover failure/success on the original Telnet/MCCP socket.

Run with an absolute flatfile server path; all state is disposable.
"""
import os
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time
import zlib
import test_flatfile_combat_journey as journey


class Client(journey.MudClient):
    def __init__(self, port):
        super().__init__(port)
        self.inflater = None
        self.waiting_marker = False
        self.wire = b''
        self.compressed_bytes = 0

    def enable_compression(self):
        self.waiting_marker = True
        self.socket.sendall(bytes([255, 253, 86]))
        self.send('look')
        self.expect('The Regression Arena')
        assert self.inflater is not None, 'MCCP was not negotiated'

    def _receive(self):
        try: data = self.socket.recv(65536)
        except socket.timeout: return False
        assert data, 'server closed the original gameplay connection'
        plain = b''
        if self.waiting_marker:
            self.wire += data
            marker = bytes([255, 250, 86, 255, 240])
            at = self.wire.find(marker)
            if at < 0: return True
            plain, data = self.wire[:at], self.wire[at + len(marker):]
            self.wire = b''
            self.waiting_marker = False
            self.inflater = zlib.decompressobj()
        if self.inflater:
            self.compressed_bytes += len(data)
            plain += self.inflater.decompress(data)
            if self.inflater.eof:
                plain += self.inflater.unused_data
                self.inflater = None
        else: plain += data
        clean = journey.ANSI.sub(b'', plain)
        self.pending.extend(clean); self.transcript.extend(clean)
        return True


def run(binary, compressed, nonroot=False):
    with tempfile.TemporaryDirectory(prefix='duris-copyover-journey-') as temporary:
        root = Path(temporary)
        state, runtime = root / 'state', root / 'runtime'
        state.mkdir(mode=0o700); (state / 'domains').mkdir(mode=0o700)
        runtime.mkdir(); (runtime / 'logs/log').mkdir(parents=True)
        (runtime / 'logs/log/.gitignore').write_text('*\n')
        journey.make_fixture(runtime); journey.generate_certificate(runtime)
        for name in ('players', 'critical'):
            (runtime / 'journals' / name).mkdir(parents=True, mode=0o700)
        (runtime / 'bin/server').mkdir(parents=True)
        shutil.copy2(binary, runtime / 'bin/server/dms')
        shutil.copy2(binary, runtime / 'bin/server/dms_new')
        path = root / 'copyover-state/copyover.dat'
        port, tls, websocket = journey.available_ports()
        env = dict(PATH=os.environ.get('PATH', '/usr/bin:/bin'), ENVIRONMENT='local',
                   PERSISTENCE_MODE='flatfile-primary', FLATFILE_STATE_DIR=str(state),
                   PLAYER_SAVE_JOURNAL_DIR=str(runtime / 'journals/players'),
                   CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / 'journals/critical'),
                   COPYOVER_STATE_FILE=str(path), LISTEN_ADDRESS='127.0.0.1',
                   DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_LISTEN_ADDRESS='127.0.0.1',
                   DURIS_WEBSOCKET_PORT=str(websocket), REDIS='FALSE', CHAOS_MUD='FALSE')
        subprocess.run([str(journey.INSPECTOR), str(state), 'seed-combat'], check=True)
        process = output = client = None
        fixture_user = None

        def stop():
            nonlocal process, output, client
            if client: client.close(); client = None
            if process and process.poll() is None:
                process.terminate(); process.wait(timeout=30)
            if output: output.close()

        def boot():
            nonlocal process, output
            output = (runtime / 'server.out').open('w')
            process = subprocess.Popen([str(runtime / 'bin/server/dms'), '--minimal', '-s', str(port)],
                                       cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT,
                                       user=fixture_user, group=fixture_user)
            deadline = time.monotonic() + 90
            while 'Entering game loop.' not in (runtime / 'server.out').read_text(errors='replace'):
                assert process.poll() is None and time.monotonic() < deadline, 'boot failed'
                time.sleep(0.1)

        try:
            boot(); client = Client(port); journey.create_character(client)
            client.send('save'); client.expect(f'Save complete for {journey.CHARACTER}.')
            client.send('quit'); client.expect('ACCOUNT MENU', timeout=30)
            stop()
            if nonroot:
                assert os.geteuid() == 0, '--nonroot fixture must start as root to drop to UID 10001'
                root.chmod(0o755)
                for directory, _, files in os.walk(root, followlinks=False):
                    os.chown(directory, 10001, 10001)
                    for name in files:
                        file = Path(directory) / name
                        if not file.is_symlink(): os.chown(file, 10001, 10001)
                os.chown(runtime, 0, 0); runtime.chmod(0o755)
                fixture_user = 10001
                assert runtime.stat().st_uid == 0 and runtime.stat().st_mode & 0o777 == 0o755
            boot()
            original = journey.MudClient
            journey.MudClient = Client
            try: client = journey.reconnect_character(port)
            finally: journey.MudClient = original
            if compressed: client.enable_compression()
            process.send_signal(signal.SIGUSR1)
            client.expect('Copyover FAILED', timeout=60)
            assert not path.exists() and process.poll() is None
            client.send('look'); client.expect('The Regression Arena')
            client.send('save'); client.expect(f'Save complete for {journey.CHARACTER}.', timeout=30)
            if compressed: assert client.inflater is not None and client.compressed_bytes > 0
            print(f'{compressed=}: failed open kept transport and save worker usable', flush=True)
            path.parent.mkdir()
            if nonroot: os.chown(path.parent, 10001, 10001)
            with socket.create_connection(('127.0.0.1', port), timeout=10) as idle:
                idle.settimeout(10)
                idle.recv(65536)
                process.send_signal(signal.SIGUSR1)
                client.expect('Copyover cancelled: a connection cannot survive this handoff', timeout=30)
                assert process.poll() is None and not path.exists()
                client.send('look'); client.expect('The Regression Arena')
                idle.sendall(b'\n')
                assert idle.recv(65536), 'copyover closed a non-preservable connection'
            process.send_signal(signal.SIGUSR1); client.expect('Copyover complete!', timeout=90)
            client.send('look'); client.expect('The Regression Arena')
            client.send('save'); client.expect(f'Save complete for {journey.CHARACTER}.', timeout=30)
            assert not path.exists() and process.poll() is None
            assert (runtime / 'server.out').read_text().count('Entering game loop.') == 2
            print(f'{compressed=}, {nonroot=}: actual exec recovered the original socket, look and acknowledged save', flush=True)
        except Exception:
            print((runtime / 'server.out').read_text(errors='replace')[-7000:])
            print(journey.runtime_logs(runtime))
            if client: print(client.transcript.decode(errors='replace')[-6000:])
            raise
        finally: stop()


if __name__ == '__main__':
    for compressed in (False, True): run(Path(sys.argv[1]).resolve(), compressed, '--nonroot' in sys.argv)
