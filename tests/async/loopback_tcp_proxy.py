#!/usr/bin/env python3
"""Expose one test-owned TCP endpoint on loopback without inspecting traffic."""

from __future__ import annotations

import os
from pathlib import Path
import socket
import sys
import threading


def copy(source: socket.socket, destination: socket.socket) -> None:
    try:
        while payload := source.recv(65536):
            destination.sendall(payload)
    except OSError:
        pass
    finally:
        try:
            destination.shutdown(socket.SHUT_WR)
        except OSError:
            pass


def serve(client: socket.socket, upstream_host: str, upstream_port: int) -> None:
    try:
        with client, socket.create_connection((upstream_host, upstream_port), timeout=10) as upstream:
            upstream.settimeout(None)
            inbound = threading.Thread(target=copy, args=(client, upstream), daemon=True)
            outbound = threading.Thread(target=copy, args=(upstream, client), daemon=True)
            inbound.start()
            outbound.start()
            inbound.join()
            outbound.join()
    except OSError:
        client.close()


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: loopback_tcp_proxy.py UPSTREAM_HOST UPSTREAM_PORT READY_FILE")
    upstream_host, raw_port, ready_name = sys.argv[1:]
    upstream_port = int(raw_port)
    if not 1 <= upstream_port <= 65535:
        raise SystemExit("upstream port is outside 1..65535")
    ready_path = Path(ready_name)
    with socket.socket() as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", 0))
        listener.listen(16)
        ready_path.write_text(str(listener.getsockname()[1]) + "\n")
        os.chmod(ready_path, 0o600)
        while True:
            client, _ = listener.accept()
            threading.Thread(target=serve, args=(client, upstream_host, upstream_port),
                             daemon=True).start()


if __name__ == "__main__":
    main()
