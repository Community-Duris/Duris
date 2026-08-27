#!/usr/bin/env python3
"""Source contracts for the active-player epic bonus hot path."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def function_body(text: str, signature: str, last: bool = False) -> str:
    start = text.rindex(signature) if last else text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    raise ValueError(f"unterminated function: {signature}")


bonus = (SRC / "epic_bonus.c").read_text()
player = (SRC / "sql_player.c").read_text()
epic = (SRC / "epic.c").read_text()
state_header = (SRC / "epic_bonus_state.h").read_text()
structs = (SRC / "structs.h").read_text()

read_body = function_body(bonus, "float get_epic_bonus(P_char ch, int type)")
selection_body = function_body(bonus, "void epic_bonus_set(P_char ch, int type)")
hydration_body = function_body(bonus, "bool epic_bonus_hydrate(P_char ch)")
record_body = function_body(bonus, "void epic_bonus_record_gain(P_char ch, int type, int amount)")
load_body = function_body(player, "P_char sql_load_player(const char *name)", last=True)
gain_body = function_body(epic, "void gain_epic(P_char ch, int type, int data, int amount)")
award_ack_body = function_body(epic, "void epic_award_committed(")

forbidden = ["qry(", "db_query(", "mysql_", "redis", "fopen", "open(", "malloc",
             "new ", "pthread_", "epic_bonus_hydrate"]
caller_files = {
    path.relative_to(ROOT).as_posix()
    for path in SRC.rglob("*.c")
    if path.name != "epic_bonus.c" and "get_epic_bonus(" in path.read_text(errors="ignore")
}
expected_callers = {"src/actinf.c", "src/epic.c", "src/limits.c", "src/shop.c",
                    "src/ships/ship_shop.c"}

checks = [
    ("player owns trivial cache", "struct EpicBonusState epic_bonus_state;" in structs),
    ("cache is fixed capacity", "EPIC_BONUS_STATE_MAX_BUCKETS 32" in state_header),
    ("read delegates only to state", "epic_bonus_state_modifier" in read_body),
    ("read has no external or lazy work", all(token not in read_body for token in forbidden)),
    ("read refreshes cap and rejects window drift", "contribution_cap" in read_body and "state->window_days != window_days" in read_body),
    ("all active caller files inventoried", caller_files == expected_callers),
    ("hydration is one grouped query", hydration_body.count("db_query(") == 1 and "GROUP BY" in hydration_body and "ORDER BY" in hydration_body),
    ("hydration includes legacy and ledger non-bottle positive gains", "type != %d AND epics > 0" in hydration_body and "reason_type != %d AND delta > 0" in hydration_body and "EPIC_BOTTLE" in hydration_body),
    ("hydration uses selection and rolling cutoffs", "gained.time > eb.time" in hydration_body and "DATE_SUB(CURDATE()" in hydration_body),
    ("midnight expiry preserves strict cutoff", "TIME(gained.time) = '00:00:00'" in hydration_body and "exact_midnight" in bonus),
    ("hydration has explicit unavailable outcomes", hydration_body.count("epic_bonus_state_mark_unavailable") >= 5),
    ("login hydrates after status", load_body.index("sql_load_player_status") < load_body.index("sql_load_player_epic_bonus")),
    ("selection persists before cache publication", selection_body.index("if (!qry(") < selection_body.index("epic_bonus_state_select")),
    ("selection write is idempotent", "ON DUPLICATE KEY UPDATE" in selection_body),
    ("award submits immutable final amount", "epic_transaction_submit(ch, amount" in gain_body),
    ("award cache updates only from committed ack", "if (!committed" in award_ack_body and "epic_bonus_record_gain(ch, context.type, context.amount);" in award_ack_body),
    ("live gain preserves strict selection cutoff", "now <= state->selected_at" in record_body),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)
print("\nEpic bonus hot-path contracts passed.")
