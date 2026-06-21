#!/usr/bin/env python3
from pathlib import Path
import sys

src = Path(__file__).resolve().parents[2] / 'src' / 'auction_houses.c'
text = src.read_text()

ok = True

# auction_pickup quantity chain safety
pickup_start = text.find('bool auction_pickup(P_char ch, char *args)')
if pickup_start == -1:
    print('[FAIL] auction_pickup not found')
    ok = False
else:
    next_check = text.find('if (!temp_obj->next_content)', pickup_start)
    old_check = text.find('if (!temp_obj)', pickup_start)
    if next_check == -1:
        print('[FAIL] auction_pickup still uses the wrong null check for chained objects')
        ok = False
    else:
        print('[PASS] auction_pickup checks temp_obj->next_content before advancing the chain')
    if old_check != -1 and old_check < next_check:
        print('[FAIL] stale temp_obj null-check still appears in auction_pickup')
        ok = False

# finalize_auction transaction wrapper and ordering
final_start = text.find('bool finalize_auction(int auction_id, P_char to_ch)')
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

# auction_bid buy-it-now failure handling
bid_start = text.find('bool auction_bid(P_char ch, char *args)')
if bid_start == -1:
    print('[FAIL] auction_bid not found')
    ok = False
else:
    finalize_check = text.find('if (!finalize_auction(auction_id, ch))', bid_start)
    refund_check = text.find('ADD_MONEY(ch, to_pay);', bid_start)
    if finalize_check == -1 or refund_check == -1 or refund_check < finalize_check:
        print('[FAIL] auction_bid does not refund on finalize_auction failure')
        ok = False
    else:
        print('[PASS] auction_bid refunds the buyer if finalize_auction fails')

sys.exit(0 if ok else 1)
