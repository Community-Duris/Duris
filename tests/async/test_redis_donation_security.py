#!/usr/bin/env python3
"""Runtime and source-contract tests for authenticated Redis donation events."""

from __future__ import annotations

from _paths import SRC
import hashlib
import hmac
import json
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SECRET = "donation-test-secret-that-is-at-least-32-bytes"
NOW = 2_000_000_000


def signed_event(**updates: object) -> str:
    event: dict[str, object] = {
        "schema_version": 1,
        "event_id": "evt_1234567890abcdef",
        "issued_at": NOW,
        "amount_cents": 1250,
        "currency": "USD",
        "is_public": True,
        "character_name": "Apex",
        "message": "Thank you",
    }
    event.update(updates)
    canonical = "\n".join(
        (
            "v1",
            str(event["event_id"]),
            str(event["issued_at"]),
            str(event["amount_cents"]),
            str(event["currency"]),
            "1" if event["is_public"] else "0",
            str(event.get("character_name", "")),
            str(event.get("message", "")),
        )
    )
    event["signature"] = hmac.new(
        SECRET.encode(), canonical.encode(), hashlib.sha256
    ).hexdigest()
    return json.dumps(event, separators=(",", ":"))


def main() -> None:
    redis = (SRC / "redis.c").read_text(encoding="utf-8")
    runtime = (SRC / "redis_donation_runtime.c").read_text(encoding="utf-8")
    worker = (SRC / "redis_donation_worker.c").read_text(encoding="utf-8")
    worker_header = (SRC / "redis_donation_worker.h").read_text(encoding="utf-8")
    events = (SRC / "new_events.c").read_text(encoding="utf-8")
    assert 'getenv("REDIS_DONATION_SUBSCRIBER")' in redis
    assert 'getenv("REDIS_DONATION_SECRET")' in redis
    assert "redis_donation_runtime_enabled()" in events
    assert "REDIS_DONATION_MAX_MESSAGES_PER_PULSE" in runtime
    assert "redis_donation_worker_take" in runtime
    assert "wait_for_retry(reconnect_delay_seconds)" in worker
    assert "REDIS_DONATION_REPLAY_CAPACITY" in worker
    assert "REDIS_DONATION_QUEUE_CAPACITY" in worker_header
    assert "REDIS_DONATION_WORK_BATCH" in worker_header

    harness = r'''
#include "economy/donation_event.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    std::string payload;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), stdin))
        payload += buffer;
    struct donation_event event = {};
    if (!donation_event_decode(payload.data(), payload.size(), argv[1], atoll(argv[2]), &event))
        return 1;
    printf("%s|%lld|%s|%s|%s\n", event.event_id, (long long)event.amount_cents,
           event.currency, event.character_name, event.message);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="donation-security-") as temp_dir:
        temp = Path(temp_dir)
        harness_path = temp / "harness.cpp"
        binary = temp / "harness"
        harness_path.write_text(harness, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(SRC),
                str(SRC / "donation_event.c"),
                str(harness_path),
                "-lcjson",
                "-lcrypto",
                "-o",
                str(binary),
            ],
            check=True,
        )

        def decode(payload: str) -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [str(binary), SECRET, str(NOW)],
                input=payload,
                text=True,
                capture_output=True,
                check=False,
            )

        valid = decode(signed_event())
        assert valid.returncode == 0, valid
        assert valid.stdout == "evt_1234567890abcdef|1250|USD|Apex|Thank you\n"

        cases = [
            signed_event(amount_cents=0),
            signed_event(amount_cents=100_000_001),
            signed_event(currency="usd"),
            signed_event(issued_at=NOW - 301),
            signed_event(character_name="Apex&+R"),
            signed_event(message="forged\nlog line"),
            signed_event(message="not-ascii-\u20ac"),
            signed_event(event_id="short"),
            signed_event() + (" " * 4096),
        ]
        tampered = json.loads(signed_event())
        tampered["amount_cents"] = 999_999
        cases.append(json.dumps(tampered))
        for payload in cases:
            result = decode(payload)
            assert result.returncode == 1, (payload, result)

    print("redis donation security tests passed")


if __name__ == "__main__":
    main()
