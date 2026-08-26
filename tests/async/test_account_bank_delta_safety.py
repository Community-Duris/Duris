#!/usr/bin/env python3
"""Account-bank delta and committed-publication source contracts."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
sql = (root / "src/sql_player.c").read_text()
header = (root / "src/sql_player.h").read_text()
utility = (root / "src/utility.c").read_text()
actoth = (root / "src/actoth.c").read_text()
boon = (root / "src/boon.c").read_text()
ships = (root / "src/ships/ship_base.c").read_text()


def section(text: str, start_marker: str, end_marker: str) -> str:
    start = text.rfind(start_marker)
    assert start >= 0, start_marker
    end = text.find(end_marker, start)
    assert end >= 0, end_marker
    return text[start:end]


all_sources = "\n".join(
    path.read_text(errors="replace") for path in (root / "src").rglob("*.[ch]")
)
assert "sql_save_account_bank" not in all_sources
assert "set bank_copper=%d" not in sql
print("[PASS] cached absolute account-bank save API and write are gone")

assert "struct AccountBankBalances" in header
assert "sql_account_bank_deposit_balances" in header
assert "sql_account_bank_withdraw_value" in header
parser = section(
    sql,
    "static bool sql_parse_account_bank_balance(const char *value, int *balance)\n{",
    "static bool sql_read_account_bank_balances",
)
assert "errno == ERANGE" in parser
assert "parsed < 0 || parsed > INT_MAX" in parser
assert "*balance = (int)parsed;" in parser
print("[PASS] committed balances are strict and legacy-cache representable")

deposit = section(
    sql,
    "bool sql_account_bank_deposit_balances(const char *account_name, int racewar,",
    "long long sql_account_bank_deposit(",
)
deposit_checks = {
    "owns transaction": "sql_in_transaction()" in deposit
    and "sql_begin_transaction()" in deposit,
    "checks ensure": "if (!sql_ensure_account_bank" in deposit,
    "uses arithmetic deltas": "bank_copper=bank_copper+%d" in deposit
    and "bank_platinum=bank_platinum+%d" in deposit,
    "checks update and affected row": "if (!sql_run_query(query) || mysql_affected_rows(DB) != 1)"
    in deposit,
    "queries result before commit": deposit.index("sql_read_account_bank_balances")
    < deposit.index("sql_commit()"),
    "publishes output after commit": deposit.index("sql_commit()")
    < deposit.index("*committed = result;"),
    "rolls back every transaction failure": deposit.count("sql_account_bank_rollback();")
    >= 3,
}
for label, passed in deposit_checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] deposit: {label}")
assert all(deposit_checks.values())

withdraw = section(
    sql,
    "long long sql_account_bank_withdraw(const char *account_name, int racewar, int coin_type,",
    "int sql_account_bank_withdraw_value(",
)
update = withdraw.index("update account_banks set %s = %s - %d")
read = withdraw.index("sql_read_account_bank_balances")
commit = withdraw.index("sql_commit()")
returned = withdraw.index("return sql_account_bank_selected_balance")
assert "if (!sql_ensure_account_bank" in withdraw
assert "and %s >= %d" in withdraw
assert "mysql_affected_rows(DB) != 1" in withdraw
assert "return row_exists ? -2 : -1;" in withdraw
assert update < read < commit < returned
print("[PASS] guarded withdrawal returns only its post-update committed balance")

aggregate = section(
    sql,
    "int sql_account_bank_withdraw_value(const char *account_name, int racewar, int amount,",
    "#endif // __NO_MYSQL__",
)
assert "true, &current" in aggregate
assert "total < amount" in aggregate and "return -2;" in aggregate
assert "bank_copper=bank_copper-%d" in aggregate
assert "bank_platinum=bank_platinum-%d" in aggregate
assert aggregate.index("sql_read_account_bank_balances(esc_name, racewar, true") < aggregate.index(
    "update account_banks"
)
assert aggregate.index("sql_commit()") < aggregate.index("*committed = result;")
assert "*change = -remaining;" in aggregate
print("[PASS] aggregate payment locks authoritative state and applies one delta vector")

single_publish = section(
    utility,
    "void publish_account_bank_balance(const char *account_name, int racewar, int coin_type,",
    "void publish_account_bank_balances(",
)
vector_publish = section(
    utility,
    "void publish_account_bank_balances(const char *account_name, int racewar,",
    "int SUB_BALANCE(",
)
for publisher in (single_publish, vector_publish):
    assert "descriptor_list" in publisher
    assert "desc->connected != CON_PLAYING" in publisher
    assert "strcasecmp(desc->account->acct_name, account_name)" in publisher
    assert "GET_RACEWAR(target) != racewar" in publisher
    assert "gmcp_char_vitals(target);" in publisher
assert all(
    field in vector_publish
    for field in (
        "GET_BALANCE_COPPER(target) = balances->copper;",
        "GET_BALANCE_SILVER(target) = balances->silver;",
        "GET_BALANCE_GOLD(target) = balances->gold;",
        "GET_BALANCE_PLATINUM(target) = balances->platinum;",
    )
)
print("[PASS] committed results reach every playing same-account/same-side character")

deposit_caller = section(actoth, "static bool deposit_carried_coin(", "void do_deposit(")
assert deposit_caller.index("sql_account_bank_deposit(") < deposit_caller.index(
    "change_carried_coin(ch, coin_type, -amount);"
)
assert "publish_account_bank_balance" in deposit_caller
assert "mark_player_dirty" in deposit_caller
do_deposit = section(actoth, "void do_deposit(", "void do_withdraw(")
assert "GET_BALANCE_COPPER(ch) +=" not in do_deposit
assert "coins were retained" in do_deposit and "failed denomination was retained" in do_deposit
do_withdraw = section(actoth, "void do_withdraw(", "void do_sneak(")
assert do_withdraw.index("sql_account_bank_withdraw(") < do_withdraw.index(
    "change_carried_coin(ch, ctype, money);"
)
assert "bank_result == -2" in do_withdraw
assert "The bank could not complete that withdrawal" in do_withdraw
print("[PASS] ATM callers mutate wallets and report success only after durable success")

sub_balance = section(utility, "int SUB_BALANCE(", "int SUB_MONEY(")
assert "GET_BALANCE(ch)" not in sub_balance
assert "sql_account_bank_withdraw_value" in sub_balance
assert sub_balance.index("sql_account_bank_withdraw_value") < sub_balance.index(
    "publish_account_bank_balances"
)
assert "ADD_MONEY(ch, change);" in sub_balance
assert "sql_save_account_bank" not in sub_balance
print("[PASS] aggregate payments do not trust or absolute-save a character cache")

cash_boon = section(boon, "case BTYPE_CASH:", "case BTYPE_LEVEL:")
assert "sql_account_bank_deposit_balances" in cash_boon
assert cash_boon.index("sql_account_bank_deposit_balances") < cash_boon.index(
    "Your bank receives a deposit"
)
assert "GET_BALANCE_" not in cash_boon
insurance = section(ships, "int insurance = 0;", "int old_class = ship->m_class;")
assert "sql_account_bank_deposit" in insurance
assert "publish_account_bank_balance" in insurance
assert "GET_BALANCE_PLATINUM(owner) +=" not in insurance
assert "insert_money_pickup" in insurance
print("[PASS] boon and ship reward paths use checked deltas without false success")

stub = sql[: sql.index("#else")]
assert "sql_account_bank_deposit_balances" in stub
assert "sql_account_bank_withdraw_value" in stub
assert "return false;" in stub and "return -1;" in stub
print("[PASS] no-MySQL bank helpers fail closed")

print("account-bank delta safety source contracts passed")
