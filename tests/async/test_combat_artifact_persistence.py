#!/usr/bin/env python3
"""Combat and artifact persistence-boundary source contracts."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
fight_text = (root / "src/fight.c").read_text()
sql_text = (root / "src/sql.c").read_text()
sql_header = (root / "src/sql.h").read_text()
artifact_text = (root / "src/artifact.c").read_text()


def function(text: str, signature: str, next_signature: str) -> str:
    start = text.rfind(signature)
    assert start >= 0, signature
    end = text.find(next_signature, start)
    assert end >= 0, next_signature
    return text[start:end]


add_frags = function(fight_text, "void AddFrags(P_char ch, P_char victim)\n{", "void update_pos")
loss_calc = "loss = gain;"
loss_mutation = "victim->only.pc->frags -= loss;"
loss_write = "sql_modify_frags(victim, -loss);"
loss_cache = "redis_invalidate_fraglist();"
loss_message = (
    'snprintf(buffer, sizeof buffer, "You just lost %.02f frags!\\r\\n", '
    "((float)loss) / 100);"
)
victim_loss = add_frags[add_frags.rindex(loss_calc):]
assert victim_loss.index(loss_calc) < victim_loss.index(loss_mutation)
assert victim_loss.index(loss_mutation) < victim_loss.index(loss_write)
assert victim_loss.index(loss_write) < victim_loss.index(loss_cache)
assert victim_loss.index(loss_cache) < victim_loss.index(loss_message)
assert add_frags.count(loss_mutation) == 1
assert add_frags.count(loss_write) == 1
print("[PASS] victim frag mutation precedes durable publication and cache invalidation")

assert "bool sql_get_bind_data(int vnum, int *owner_pid, int *timer);" in sql_header
bind_lookup = function(
    sql_text,
    "bool sql_get_bind_data(int vnum, int *owner_pid, int *timer)\n{",
    "void sql_update_bind_data",
)
query = 'qry("SELECT owner_pid, timer FROM artifact_bind WHERE vnum = %d", vnum)'
query_failure = bind_lookup[bind_lookup.index(f"if (!{query})"):bind_lookup.index(
    "MYSQL_RES *res"
)]
allocation_failure = bind_lookup[bind_lookup.index("if (!res)"):bind_lookup.index(
    "if (mysql_num_rows"
)]
malformed_failure = bind_lookup[bind_lookup.index("if (!row ||"):bind_lookup.index(
    "*owner_pid = parsed_owner_pid;"
)]
checks = {
    "rejects invalid output pointers": "if (!owner_pid || !timer)" in bind_lookup,
    "initializes owner before query": bind_lookup.index("*owner_pid = 0;")
    < bind_lookup.index(query),
    "initializes timer before query": bind_lookup.index("*timer = 0;")
    < bind_lookup.index(query),
    "initializes provided outputs before pointer rejection": bind_lookup.index(
        "*timer = 0;"
    )
    < bind_lookup.index("if (!owner_pid || !timer)"),
    "query failure is explicit": "failed to read from database" in query_failure
    and "return false;" in query_failure,
    "allocation failure is checked": "if (!res)" in bind_lookup
    and "mysql_store_result failed" in allocation_failure
    and "return false;" in allocation_failure,
    "no row is successful defaults": "if (mysql_num_rows(res) < 1)" in bind_lookup
    and "mysql_free_result(res);\n\t\treturn true;" in bind_lookup,
    "row fetch is checked": "if (!row ||" in bind_lookup,
    "both columns are strictly parsed": "sql_parse_bind_int(row[0]" in bind_lookup
    and "sql_parse_bind_int(row[1]" in bind_lookup,
    "malformed row is explicit failure": "malformed database row" in malformed_failure
    and "return false;" in malformed_failure,
    "values publish atomically": bind_lookup.index("int parsed_owner_pid = 0;")
    < bind_lookup.index("*owner_pid = parsed_owner_pid;")
    and bind_lookup.index("int parsed_timer = 0;")
    < bind_lookup.index("*timer = parsed_timer;"),
    "valid row returns success after publication": bind_lookup.index(
        "*timer = parsed_timer;"
    )
    < bind_lookup.rindex("return true;"),
}
for label, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] bind lookup: {label}")
assert all(checks.values())

parser = function(sql_text, "static bool sql_parse_bind_int", "bool sql_get_bind_data")
assert "!isdigit((unsigned char)*digit)" in parser
assert "errno == ERANGE" in parser
assert "parsed < INT_MIN || parsed > INT_MAX" in parser
assert "*result = (int)parsed;" in parser
print("[PASS] malformed and out-of-range bind integers cannot publish")

stub_start = sql_text.index("bool sql_get_bind_data(int vnum, int *owner_pid, int *timer)\n{")
stub_end = sql_text.index("bool sql_pwipe", stub_start)
stub = sql_text[stub_start:stub_end]
assert "*owner_pid = 0;" in stub
assert "*timer = 0;" in stub
assert "return false;" in stub
print("[PASS] no-MySQL bind lookup initializes outputs and reports failure")

assert artifact_text.count("sql_get_bind_data(") == 3
assert artifact_text.count("if (!sql_get_bind_data(") == 3
for caller, next_caller in (
    ("void artifact_switch_check", "void artifact_update_sql"),
    ("void artifact_feed_sql", "void poof_artifact"),
    ("void arti_fixit_sql", "void arti_sync_sql"),
):
    body = function(artifact_text, caller, next_caller)
    failure = body.index("if (!sql_get_bind_data(")
    failure_block = body[failure:body.index("}", failure) + 1]
    assert "return;" in failure_block or "continue;" in failure_block
print("[PASS] all artifact bind callers fail closed before ownership decisions")

print("combat and artifact persistence source contracts passed")
