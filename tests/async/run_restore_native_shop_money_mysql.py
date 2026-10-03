#!/usr/bin/env python3
"""Read-only restore checks after a genuine buy on an owned disposable schema."""
import os
from pathlib import Path
import subprocess
import sys

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from qualify_database_restore import require_currency_revision_history, require_currency_values

if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET") or
        not os.environ.get("DB_NAME", "").startswith("economic_schema_test_")):
    raise SystemExit("explicit owned disposable loopback schema required")

settings = {"host": "127.0.0.1", "port": int(os.environ["DB_PORT"]),
            "user": os.environ["DB_USER"], "password": os.environ["DB_PASSWD"],
            "database": os.environ["DB_NAME"], "autocommit": True,
            "connect_timeout": 5, "read_timeout": 30}
connection = pymysql.connect(**settings)
reader = None
PID = 2147000731
ITEM = 9900000731
if sys.argv[1:] not in ([], ["--full-trade"]):
    raise SystemExit("usage: run_restore_native_shop_money_mysql.py [--full-trade]")
purchase_only = not sys.argv[1:]


def execute(query, args=None):
    with connection.cursor() as cursor:
        cursor.execute(query, args)


def scalar(query):
    with connection.cursor() as cursor:
        cursor.execute(query)
        return cursor.fetchone()[0]


class Executor:
    def sql(self, query):
        assert query.startswith("SELECT ")
        with reader.cursor() as cursor:
            cursor.execute(query)
            return str(cursor.fetchone()[0])


def admitted():
    require_currency_revision_history(Executor())
    require_currency_values(Executor())


def refused():
    try:
        admitted()
    except RuntimeError as error:
        assert str(error) == "restore_currency_value_mismatch", error
    else:
        raise AssertionError("corrupt native/economic money history admitted")


def capture():
    queries = (
        "SELECT pid,copper,silver,gold,platinum,wallet_revision FROM player_data",
        "SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision FROM account_banks",
        "SELECT HEX(operation_id),HEX(account_key),before_copper,before_silver,before_gold,"
        "before_platinum,after_copper,after_silver,after_gold,after_platinum,before_revision,"
        "after_revision FROM economic_accounting_account_effect ORDER BY account_key",
        "SELECT HEX(operation_id),status,result_code,HEX(command_hash),HEX(keys_hash),"
        "SHA2(result_payload,256) FROM critical_operation_inbox ORDER BY operation_id",
    )
    with connection.cursor() as cursor:
        values = []
        for query in queries:
            cursor.execute(query)
            values.append(cursor.fetchall())
        return values


try:
    # The parent provisions and independently selects an empty fully migrated
    # daemon. Never run this fixture on an existing owner, even in a test schema.
    for table in ("player_data", "account_banks", "currency_ledger",
                  "economic_accounting_operation"):
        assert scalar("SELECT COUNT(*) FROM " + table) == 0, table
    env = dict(os.environ, DURIS_RESTORE_SHOP_PURCHASE_CUT="1" if purchase_only else "0")
    subprocess.run([sys.executable, str(ROOT / "tests/async/run_shop_trade_sql_lock_mysql.py")],
                   cwd=ROOT, env=env, check=True)
    assert scalar("SELECT wallet_revision FROM player_data") == (5 if purchase_only else 6)
    assert scalar("SELECT bank_revision FROM account_banks") == (8 if purchase_only else 9)
    assert scalar("SELECT COUNT(*) FROM currency_ledger") == 0
    assert scalar("SELECT COUNT(*) FROM economic_accounting_operation WHERE outcome=1") == (1 if purchase_only else 2)
    expected_owner = PID if purchase_only else 4
    assert scalar(f"SELECT owner_id FROM item_current_owner WHERE item_uid={ITEM}") == expected_owner
    bank = scalar("SELECT id FROM account_banks")
    # The opening cut is the native fixture's pre-purchase authority, not its
    # changed after-image. No accounting root or receipt is fabricated here.
    execute("INSERT INTO currency_wallet_baseline(pid,opening_copper,opening_silver,"
            "opening_gold,opening_platinum,opening_revision) VALUES(%s,0,0,0,1,4)", (PID,))
    execute("INSERT INTO currency_bank_baseline(bank_id,opening_copper,opening_silver,"
            "opening_gold,opening_platinum,opening_revision) VALUES(%s,2,0,0,0,7)", (bank,))
    reader = pymysql.connect(**settings)
    with reader.cursor() as cursor:
        cursor.execute("SET SESSION TRANSACTION READ ONLY")
    before = capture()
    admitted()
    wallet_key = scalar("SELECT account_key FROM economic_accounting_account_effect "
                        "WHERE before_revision=4 AND after_revision=5")
    wallet_operation = scalar("SELECT operation_id FROM economic_accounting_account_effect "
                              "WHERE before_revision=4 AND after_revision=5")
    execute("UPDATE player_data SET gold=9")
    refused()
    execute("UPDATE player_data SET gold=%s", (8 if purchase_only else 0,))
    admitted()
    execute("UPDATE account_banks SET bank_copper=NULL")
    refused()
    execute("UPDATE account_banks SET bank_copper=2")
    admitted()
    execute("UPDATE economic_accounting_account_effect SET before_platinum=0 WHERE account_key=%s "
            "AND before_revision=4 AND after_revision=5",
            (wallet_key,))
    refused()
    execute("UPDATE economic_accounting_account_effect SET before_platinum=1 WHERE account_key=%s "
            "AND before_revision=4 AND after_revision=5",
            (wallet_key,))
    admitted()
    # A consistent but out-of-range vector still cannot be native money: its
    # weighted copper total exceeds signed 64-bit, without SQL arithmetic wrap.
    execute("UPDATE currency_wallet_baseline SET opening_platinum=9223372036854775807")
    execute("UPDATE player_data SET platinum=9223372036854775807")
    execute("UPDATE economic_accounting_account_effect SET before_platinum=9223372036854775807,"
            "after_platinum=9223372036854775807 WHERE account_key=%s "
            "AND before_revision=4 AND after_revision=5", (wallet_key,))
    refused()
    execute("UPDATE currency_wallet_baseline SET opening_platinum=1")
    execute("UPDATE player_data SET platinum=%s", (0 if purchase_only else 1,))
    execute("UPDATE economic_accounting_account_effect SET before_platinum=1,after_platinum=0 "
            "WHERE account_key=%s AND before_revision=4 AND after_revision=5", (wallet_key,))
    admitted()
    # Add a synthetic matching legacy bridge to this genuine native cut. The
    # fixture owns this row; the shop itself never emitted a currency ledger leg.
    execute("INSERT INTO currency_ledger(operation_id,pid,bank_id,wallet_delta_copper,"
            "wallet_delta_silver,wallet_delta_gold,wallet_delta_platinum,bank_delta_copper,"
            "bank_delta_silver,bank_delta_gold,bank_delta_platinum,wallet_after_copper,"
            "wallet_after_silver,wallet_after_gold,wallet_after_platinum,bank_after_copper,"
            "bank_after_silver,bank_after_gold,bank_after_platinum,wallet_revision,bank_revision,"
            "reason_type,source_site) SELECT operation_id,%s,%s,0,0,8,-1,0,0,0,0,0,0,8,0,"
            "2,0,0,0,5,8,6,1 FROM economic_accounting_operation WHERE operation_id=%s",
            (PID, bank, wallet_operation))
    admitted()
    execute("UPDATE currency_ledger SET wallet_delta_gold=7")
    refused()
    execute("UPDATE currency_ledger SET wallet_delta_gold=8")
    admitted()
    # A later opening cut excludes its pre-opening evidence without summing it
    # again. This is a disposable native-money cut, not an accounting activation.
    execute("UPDATE currency_wallet_baseline SET opening_revision=5,opening_gold=8,opening_platinum=0")
    execute("UPDATE currency_bank_baseline SET opening_revision=8")
    admitted()
    execute("UPDATE currency_wallet_baseline SET opening_revision=4,opening_gold=0,opening_platinum=1")
    execute("UPDATE currency_bank_baseline SET opening_revision=7")
    admitted()
    execute("DELETE FROM currency_ledger")
    admitted()
    assert capture() == before
    assert scalar(f"SELECT owner_id FROM item_current_owner WHERE item_uid={ITEM}") == expected_owner
    print("Native shop restore: " + ("economic-only buy" if purchase_only else "economic-only buy/sell") +
          ", exact value history, bridge dedupe/conflict, "
          "opening cuts, corruption refusal and unchanged repaired authority passed")
finally:
    if reader is not None:
        reader.close()
    connection.close()
