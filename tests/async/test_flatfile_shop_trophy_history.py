#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-shop-trophy-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_shop_trophy_history_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/flatfile_shop_trophy_history_harness.cpp",
            rel("flatfile_shop_trophy_history.c"),
            rel("flatfile_store.c"),
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "state")], cwd=ROOT, check=True)

sql_source = (SRC / "sql.c").read_text()
shop_source = (SRC / "shop.c").read_text()

no_mysql = sql_source[sql_source.index("#ifdef __NO_MYSQL__") : sql_source.index("#else", sql_source.index("#ifdef __NO_MYSQL__"))]
if "return flat_sql_shop_trophy(obj);" not in no_mysql or "return flat_sql_shop_sell(ch, obj, value);" not in no_mysql:
    raise SystemExit("client-free SQL compatibility functions do not use flat shop history")

db_sell = sql_source[sql_source.index("int sql_shop_sell(P_char ch", sql_source.index("#else")) :]
db_sell = db_sell[: db_sell.index("int sql_shop_trophy(P_obj obj)")]
if "PERSISTENCE_MODE_FLATFILE_PRIMARY" not in db_sell or "flat_sql_shop_sell" not in db_sell:
    raise SystemExit("MySQL-capable flat-primary sale path still requires the database")

db_trophy = sql_source[sql_source.index("int sql_shop_trophy(P_obj obj)", sql_source.index("#else")) :]
db_trophy = db_trophy[: db_trophy.index("int sql_quest_finish")]
if "PERSISTENCE_MODE_FLATFILE_PRIMARY" not in db_trophy or "flat_sql_shop_trophy" not in db_trophy:
    raise SystemExit("MySQL-capable flat-primary trophy lookup still requires the database")

callback = shop_source[shop_source.index("static void shop_trade_completion") : shop_source.index("void push(")]
sale = callback[callback.index('act("$n sells $p.') :]
record = sale.index("sql_shop_sell(ch, object, payload.price);")
if not record < sale.index("obj_from_char(object)") or not record < sale.index("extract_obj(object, TRUE)"):
    raise SystemExit("committed flat sale history is recorded after the live object is consumed")

print("flat-file shop-trophy history runtime and routing regression passed")
