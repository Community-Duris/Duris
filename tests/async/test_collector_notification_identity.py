#!/usr/bin/env python3
"""Contract checks for identity-stable SQL Collector notification delivery."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
SQL_SOURCE = ROOT / "src" / "sql" / "sql.c"


def main() -> None:
    source = SQL_SOURCE.read_text()
    matches = list(re.finditer(
        r"bool send_to_pid_offline_deduplicated.*?(?=\nvoid send_offline_messages)",
        source,
        re.DOTALL,
    ))
    if not matches:
        raise AssertionError("SQL notification functions are missing")
    enqueue = matches[-1].group(0)
    if "GET_LOCK" in enqueue or "RELEASE_LOCK" in enqueue:
        raise AssertionError("notification enqueue must not hold an advisory lock")
    for required in (
        "offline_message_receipts",
        "message_id",
        "ON DUPLICATE KEY UPDATE",
        "INSERT IGNORE INTO offline_messages",
        "sql_run_multi_query",
    ):
        if required not in enqueue:
            raise AssertionError(f"identity-stable enqueue contract missing: {required}")

    delivery_matches = list(re.finditer(
        r"void send_offline_messages.*?(?=\nint sql_shop_sell)",
        source,
        re.DOTALL,
    ))
    if not delivery_matches:
        raise AssertionError("SQL offline delivery function is missing")
    delivery = delivery_matches[-1].group(0)
    for required in (
        "offline_message_receipts",
        "status=1",
        "status=2",
        "delivered_at",
        "message_id",
    ):
        if required not in delivery:
            raise AssertionError(f"durable offline delivery contract missing: {required}")

    print("collector notification identity contract passed")


if __name__ == "__main__":
    main()
