#!/usr/bin/env python3
"""Run only inside a new user/network namespace, with copied runtime assets."""
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
import urllib.error
import urllib.request


def main():
    candidate, binary = map(Path, sys.argv[1:])
    if not (candidate / "ISOLATED_RESTORE").is_file():
        raise RuntimeError("isolated_candidate_required")
    subprocess.run(["ip", "link", "set", "lo", "up"], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
    runtime = candidate / "runtime"
    log_path = candidate / "service.log"
    with log_path.open("wb") as log:
        process = subprocess.Popen([str(binary), "--minimal", "-d", str(runtime), "4000"],
                                   cwd=runtime, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 60
            while True:
                if process.poll() is not None:
                    raise RuntimeError("service_exited_before_ready")
                try:
                    with urllib.request.urlopen("http://127.0.0.1:4050/health", timeout=1) as response:
                        ready = response.status == 200 and json.load(response) == {
                            "status": "healthy", "persistence": "ready"}
                    if ready:
                        break
                except (urllib.error.URLError, TimeoutError, ConnectionError):
                    pass
                if time.monotonic() >= deadline:
                    raise RuntimeError("service_readiness_timeout")
                time.sleep(0.1)
            process.send_signal(signal.SIGTERM)
            if process.wait(timeout=30) != 0:
                raise RuntimeError("service_shutdown_failed")
            content = log_path.read_bytes().lower()
            if b"normal termination of game." not in content or any(token in content for token in (
                b"player save pipeline unavailable", b"player load pipeline unavailable",
                b"critical command pipeline unavailable", b"start_failed",
            )):
                raise RuntimeError("persistence_pipeline_failed")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()


if __name__ == "__main__":
    try:
        main()
    except Exception:
        print("isolated_service_qualification_failed", file=sys.stderr)
        raise SystemExit(1)

