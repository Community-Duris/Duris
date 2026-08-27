#!/usr/bin/env python3
from pathlib import Path
import sys

src = Path(__file__).resolve().parents[2] / 'src' / 'auction_houses.c'
text = src.read_text()

epic = Path(__file__).resolve().parents[2] / 'src' / 'epic.c'
epic_text = epic.read_text()

ship = Path(__file__).resolve().parents[2] / 'src' / 'ships' / 'ship_base.c'
ship_text = ship.read_text()

ok = True

# auction_pickup quantity chain safety
pickup_start = text.find('bool auction_pickup(P_char ch, char *args)')
if pickup_start == -1:
    print('[FAIL] auction_pickup not found')
    ok = False
else:
    next_check = text.find('if (!temp_obj->next_content)', pickup_start)
    old_check = text.find('if (!temp_obj)', pickup_start)
    bypass_guard = text.find('That auction item is already staged or picked up.', pickup_start)
    persist_fail = text.find('failed to persist pickup state', pickup_start)
    select_any = text.find("SELECT 1 FROM auction_item_pickups WHERE pid = '%d' AND obj_blob_str", pickup_start)
    if next_check == -1:
        print('[FAIL] auction_pickup still uses the wrong null check for chained objects')
        ok = False
    else:
        print('[PASS] auction_pickup checks temp_obj->next_content before advancing the chain')
    if old_check != -1 and old_check < next_check:
        print('[FAIL] stale temp_obj null-check still appears in auction_pickup')
        ok = False
    if bypass_guard == -1 or select_any == -1:
        print('[FAIL] auction_pickup admin bypass can still duplicate a staged pickup')
        ok = False
    else:
        print('[PASS] auction_pickup admin bypass rejects duplicate staging or re-pickup')
    if persist_fail == -1:
        print('[FAIL] auction_pickup still ignores writeCharacter() failures')
        ok = False
    else:
        print('[PASS] auction_pickup checks writeCharacter() result and logs persistence failures')

# finalize_auction transaction wrapper and ordering
final_start = text.find('bool finalize_auction(int auction_id, P_char')
if final_start == -1:
    print('[FAIL] finalize_auction not found')
    ok = False
else:
    begin = text.find('sql_begin_transaction()', final_start)
    commit = text.find('sql_commit()', final_start)
    rollback = text.find('sql_rollback()', final_start)
    pickup_insert = text.find('INSERT INTO auction_item_pickups', final_start)
    money_pickup = text.find('insert_money_pickup(', final_start)
    broadcast = text.find('ws_broadcast_auction_close', final_start)
    if begin == -1 or commit == -1 or rollback == -1:
        print('[FAIL] finalize_auction is missing transaction helper calls')
        ok = False
    else:
        print('[PASS] finalize_auction is transaction-wrapped')
    if pickup_insert != -1 and commit != -1 and pickup_insert < commit:
        print('[PASS] auction pickup staging happens before commit')
    else:
        print('[FAIL] auction pickup staging should happen before commit')
        ok = False
    if money_pickup != -1 and commit != -1 and money_pickup < commit:
        print('[PASS] money pickup staging happens before commit')
    else:
        print('[FAIL] money pickup staging happens after commit')
        ok = False
    if broadcast != -1 and commit != -1 and broadcast > commit:
        print('[PASS] web broadcast is deferred until after commit')
    else:
        print('[FAIL] web broadcast happens before commit')
        ok = False

# insert_money_pickup should be atomic upsert
helper_start = text.find('bool insert_money_pickup(int pid, int money)')
if helper_start == -1:
    print('[FAIL] insert_money_pickup not found')
    ok = False
else:
    helper_end = text.find('\nbool ', helper_start + 1)
    helper_section = text[helper_start:helper_end if helper_end != -1 else len(text)]
    upsert = helper_section.find('ON DUPLICATE KEY UPDATE money = money + VALUES(money)')
    legacy_select = helper_section.find('SELECT pid FROM auction_money_pickups')
    legacy_update = helper_section.find('UPDATE auction_money_pickups SET money = money + %d')
    if upsert == -1:
        print('[FAIL] insert_money_pickup is not using an atomic upsert')
        ok = False
    else:
        print('[PASS] insert_money_pickup uses a single atomic upsert')
    if legacy_select != -1 or legacy_update != -1:
        print('[FAIL] legacy select/update refund helper logic still present')
        ok = False

# auction_bid buy-it-now failure handling
bid_start = text.find('bool auction_bid(P_char ch, char *args)')
if bid_start == -1:
    print('[FAIL] auction_bid not found')
    ok = False
else:
    buy_start = text.find('if (buy_price > 0 && bid_value >= buy_price)', bid_start)
    finalize_check = text.find('if (!finalize_auction(auction_id, ch))', buy_start)
    refund_check = text.find('ADD_MONEY(ch, to_pay);', finalize_check if finalize_check != -1 else buy_start)
    if finalize_check == -1 or refund_check == -1 or refund_check < finalize_check:
        print('[FAIL] auction_bid does not refund on finalize_auction failure')
        ok = False
    else:
        print('[PASS] auction_bid refunds the buyer if finalize_auction fails')
    outbid_log = text.find('failed to stage refund pickup for pid', buy_start)
    if outbid_log == -1:
        print('[FAIL] auction_bid still ignores refund staging failures')
        ok = False
    else:
        print('[PASS] auction_bid logs refund staging failures')

# other insert_money_pickup callers should also check failures and use safe fallbacks
if 'currency_transaction_submit_wallet_value(' in epic_text and 'currency_reason_type::refund' in epic_text:
    print('[PASS] epic.c falls back to a transactional wallet credit when pickup staging fails')
else:
    print('[FAIL] epic.c does not use a transactional refund fallback')
    ok = False

if 'ship->money += insurance;' in ship_text and 'fell back to ship coffers' in ship_text:
    print('[PASS] ship_base.c falls back to ship coffers when pickup staging fails')
else:
    print('[FAIL] ship_base.c does not route insurance failures to ship coffers')
    ok = False

sys.exit(0 if ok else 1)
