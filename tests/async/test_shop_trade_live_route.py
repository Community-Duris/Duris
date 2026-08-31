#!/usr/bin/env python3
"""Source contract for post-commit flat-file shop object publication."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "shop.c").read_text()
TRANSACTION = (SRC / "shop_trade_transaction.c").read_text()

for token in (
    "shop_trade_find_object(payload.selected_item_uid)",
    "shop_trade_find_keeper(payload.shop_id)",
    "shop_trade_runtime_object_matches_payload(object, payload)",
    "object->loc.carrying == keeper",
    "object->loc.carrying == ch",
    "shop_trade_action::buy_existing",
    "shop_trade_action::buy_produced",
    "shop_trade_action::sell_store",
    "shop_trade_action::sell_destroy",
    "shop_trade_action::discard_invalid",
    "shop_trade_transaction_submit(ch, payload, shop_trade_completion)",
    "committed shop trade could not publish live object",
    "produced && keeper && OBJ_NOWHERE(object)",
    "extract_obj(selected, FALSE)",
    "produced_purchase_sequences",
    "shop_trade_container_accepts",
    "shop_trade_submit_produced_continuation",
    "shop_trade_submit_invalid_cleanup",
    "shop_trade_route_invalid_cleanup",
    "obj_to_obj(object, destination)",
):
    if token not in SOURCE:
        raise SystemExit(f"live shop trade route is missing {token}")
if "put(ch, object, destination" in SOURCE:
    raise SystemExit("committed shop placement still reruns the fallible put path")
if SOURCE.count("shop_trade_route_invalid_cleanup(") < 9:
    raise SystemExit("not every flat shop invalid-stock selection route is authority-gated")

buy_start = SOURCE.index("void shopping_buy(")
sell_start = SOURCE.index("void shopping_sell(")
value_start = SOURCE.index("void shopping_value(")
buy = SOURCE[buy_start:sell_start]
sell = SOURCE[sell_start:value_start]

if "That purchase is not available through flat-file persistence yet" in buy:
    raise SystemExit("flat buy still disables trusted zero-charge purchases")
if "const int64_t transaction_price = IS_TRUSTED(ch) ? 0 : sale;" not in buy:
    raise SystemExit("flat buy does not preserve trusted zero-charge behavior")
if buy.count("transaction_price") < 3:
    raise SystemExit("flat buy does not carry trusted pricing through produced continuations")

buy_submit = buy.index("shop_trade_transaction_submit(ch, payload, shop_trade_completion)")
if not buy_submit < buy.index("transact(ch, gem, keeper, sale)") < buy.index("writeShopKeeper(keeper)"):
    raise SystemExit("flat buy does not submit before legacy money/shop mutation")
flat_branch = buy.index("if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)",
                        buy.index("IS_CARRYING_N(ch)"))
clone = buy.index("selected = read_object(temp1->R_num, REAL)", flat_branch)
if not flat_branch < clone < buy_submit:
    raise SystemExit("produced clone is not staged inside the flat branch before submission")
sell_submit = sell.index("shop_trade_transaction_submit(ch, payload, shop_trade_completion)")
if not sell_submit < sell.index("sql_shop_sell(ch, temp1, sale)") < sell.index("ADD_MONEY(ch, sale)"):
    raise SystemExit("flat sale does not submit before legacy SQL/money mutation")

callback = SOURCE[SOURCE.index("static void shop_trade_completion"):SOURCE.index("void push(")]
if not callback.index("shop_trade_runtime_object_matches_payload") < callback.index(
    "obj_from_char(object)"
):
    raise SystemExit("live object moves before exact snapshot revalidation")
cleanup_branch = callback.index("if (cleanup)")
if not callback.index("shop_trade_runtime_object_matches_payload") < cleanup_branch < callback.index(
    "extract_obj(object, TRUE)", cleanup_branch
):
    raise SystemExit("invalid shop stock is extracted before committed snapshot revalidation")
if not TRANSACTION.index("currency_transaction_publish_balances(") < TRANSACTION.index(
    "completion(character, committed && published"
):
    raise SystemExit("shop callback can run before authoritative runtime publication")
for token in (
    "transfer.target_root_item_uid = payload.target_root_item_uid",
    "transfer.target_parent_item_uid = payload.target_parent_item_uid",
    "transfer.expected_target_parent_revision = payload.expected_target_parent_revision",
):
    if token not in TRANSACTION:
        raise SystemExit(f"shop completion publication is missing {token}")

print("flat-file live shop trade route contract passed")
