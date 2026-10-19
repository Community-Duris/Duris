#!/usr/bin/env python3

from _paths import SRC, extract_function, rel
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

# Committed sales record history during physical publication, before inventory
# detachment or destruction. Check each branch so one audit cannot cover both.
publication = extract_function("shop.c", "static bool shop_trade_publish_physical(")
store_start = publication.index("const bool at_target")
destroy_sale = publication[publication.index("else if (destroying)") : store_start]
store_sale = publication[store_start:]
for sale, consumption in (
    (destroy_sale, "extract_obj(object, TRUE)"),
    (store_sale, "obj_from_char(object)"),
):
    record = sale.index("sql_shop_sell(ch, object, payload.price);")
    if not record < sale.index(consumption):
        raise SystemExit(f"committed flat sale history is recorded after {consumption}")

print("flat-file shop-trophy history runtime and routing regression passed")
