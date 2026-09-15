#!/usr/bin/env python3
"""Game-thread collector list, inspect, purchase, and publication regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    interpreter = (ROOT / "src/cmd/interp.c").read_text()
    comm = (ROOT / "src/net/comm.c").read_text()
    assert '"collector",' in interpreter
    assert "CMD_Y(CMD_COLLECTOR" in interpreter
    assert "collector_service_pulse();" in comm
    assert comm.count("collector_service_player_busy") >= 3
    service = (ROOT / "src/economy/collector_service.c").read_text()
    assert "item_movement_transaction_player_busy" in service
    assert "bulk_get_player_busy" in service
    assert "COLLECTOR_PURCHASE_SAVE_FENCE_MAX" in service
    assert "remember_purchase_save_fence" in service
    assert "purchase_save_fence_overflow" in service
    assert "collector_service_player_ready" in service
    assert service.index("collector_runtime_find(listing") < service.index(
        "collector_listing_pipeline_next_request_id()"
    )

    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-service-", dir=output) as directory:
        binary = Path(directory) / "collector-service"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-O2",
                "-I",
                str(ROOT / "src"),
                str(ROOT / "src/economy/auction_room_registry.c"),
                str(ROOT / "src/economy/collector_policy.c"),
                str(ROOT / "src/economy/collector_codec.c"),
                str(ROOT / "src/economy/collector_purchase_preparation.c"),
                str(ROOT / "src/economy/collector_service.c"),
                str(ROOT / "src/player/player_snapshot_codec.c"),
                str(ROOT / "tests/async/collector_service_harness.cpp"),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
