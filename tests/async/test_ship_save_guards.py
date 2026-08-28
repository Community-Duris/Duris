from pathlib import Path
import sys
from contract_text import find, index

base = Path(__file__).resolve().parents[2]
ship_base = base / 'src/ships/ship_base.c'
ship_combat = base / 'src/ships/ship_combat.c'
ship_shop = base / 'src/ships/ship_shop.c'
ship_utils = base / 'src/ships/ship_utils.c'
sql_player = base / 'src/sql_player.c'

text_base = ship_base.read_text()
text_combat = ship_combat.read_text()
text_shop = ship_shop.read_text()
text_utils = ship_utils.read_text()
text_player = sql_player.read_text()

checks = []

rename_ship = find(text_base, 'bool rename_ship(P_char ch, char *owner_name, char *new_name)')
rename_guard = find(text_base, 'if (!write_ship(temp))', rename_ship)
rename_return = find(text_base, 'return FALSE;', rename_guard)
rename_unlink = find(text_base, 'unlink(buf);', rename_guard)
checks.append(('rename_ship guard order', rename_ship, rename_guard, rename_return, rename_unlink))

reown = find(text_base, 'bool rename_ship_owner(char *old_name, char *new_name)')
reown_guard = find(text_base, 'if (!write_ship(ship))', reown)
reown_return = find(text_base, 'return FALSE;', reown_guard)
reown_unlink = find(text_base, 'unlink(buf);', reown_guard)
checks.append(('rename_ship_owner guard order', reown, reown_guard, reown_return, reown_unlink))

dock = find(text_base, 'ship->anchor = world[to_room].number;')
dock_queue = find(text_base, 'queue_ship_save(ship, "docking anchor update");', dock)
checks.append(('dock ship queued save', dock, dock_queue, -1, -1))

summon_arrive = find(text_base, 'ship->speed = 0;', find(text_base, 'if (isname(buf, ship->ownername)'))
summon_queue = find(text_base, 'queue_ship_save(ship, "summon arrival");', summon_arrive)
checks.append(('summon arrival queued save', summon_arrive, summon_queue, -1, -1))

reward = find(text_combat, 'ship_gain_money(contacts[i].ship, ship, salvage, bounty);')
reward_queue = find(text_combat, 'queue_ship_save(contacts[i].ship, "combat reward");', reward)
checks.append(('combat reward queued save', reward, reward_queue, -1, -1))

sink = find(text_combat, 'ship_loss_on_sink(ship, attacker, frag_gain);')
sink_queue = find(text_combat, 'queue_ship_save(ship, "sink resolution");', sink)
checks.append(('sink queued save', sink, sink_queue, -1, -1))

summon_clear = find(text_shop, 'clear_cargo(ship);')
summon_clear_queue = find(text_shop, 'queue_ship_save(ship, "cargo clear during summon");', summon_clear)
checks.append(('summon clear queued save', summon_clear, summon_clear_queue, -1, -1))

sale = find(text_shop, 'update_crew(ship);')
sale_queue = find(text_shop, 'queue_ship_save(ship, "cargo sale");', sale)
checks.append(('cargo sale queued save', sale, sale_queue, -1, -1))

repair = find(text_shop, 'Thank you for for your business, it will take')
repair_queue = find(text_shop, 'queue_ship_save(ship, "repair");', repair)
checks.append(('repair queued save', repair, repair_queue, -1, -1))

swap = find(text_shop, 'ship->slot[slot2].clone(temp);')
swap_queue = find(text_shop, 'queue_ship_save(ship, "slot swap");', swap)
checks.append(('slot swap queued save', swap, swap_queue, -1, -1))

jettison = find(text_utils, 'update_ship_status(ship);')
jettison_queue = find(text_utils, 'queue_ship_save(ship, "cargo jettison");', jettison)
checks.append(('cargo jettison queued save', jettison, jettison_queue, -1, -1))

write_ship = find(text_base, 'int write_ship(P_ship ship)')
retry_pending = find(text_base, 'ship->save_pending     = true;', write_ship)
retry_delay = find(text_base, 'ship->save_retry_after = time(NULL) + 1;', retry_pending)
retry_signature = find(text_base, 'ship->save_saved_signature = ship_save_signature(ship);', write_ship)
flush_fn = find(text_base, 'void flush_pending_ship_saves(void)')
checks.append(('ship save retry fallback', write_ship, retry_pending, retry_delay, retry_signature))

# MySQL is the only ship read authority. Redis invalidation remains temporarily so
# retired snapshot keys cannot survive deletes or owner renames.
sql_load_ship_fn = find(text_player, 'P_ship sql_load_ship(const char *owner_name)')
sql_load_ship_end = find(text_player, 'bool sql_load_all_ships()', sql_load_ship_fn)
sql_load_query = find(text_player, 'from ships where owner_name', sql_load_ship_fn)
checks.append(('sql_load_ship SQL authority', sql_load_ship_fn, sql_load_query, -1, -1))

sql_delete_ship_fn = find(text_player, 'bool sql_delete_ship(const char *owner_name)')
sql_delete_query = find(text_player, "delete from ships where owner_name='%s'", sql_delete_ship_fn)
sql_delete_inv = find(text_player, 'redis_invalidate_ship_snapshot', sql_delete_ship_fn)
checks.append(('sql_delete_ship redis invalidate', sql_delete_ship_fn, sql_delete_query, sql_delete_inv, -1))

reown_inv = find(text_base, 'redis_invalidate_ship_snapshot(old_name)', reown)
checks.append(('rename_ship_owner redis invalidate', reown, reown_inv, -1, -1))

ok = True
for name, a, b, c, d in checks:
    print(f'{name}: a={a} b={b} c={c} d={d}')
    if a == -1 or b == -1:
        print(f'missing expected snippet for {name}')
        ok = False
    if name == 'rename_ship guard order' and not (b < c < d):
        print('rename_ship does not abort before unlink on write failure')
        ok = False
    if name == 'rename_ship_owner guard order' and not (b < c < d):
        print('rename_ship_owner does not abort before unlink on write failure')
        ok = False
    if name.endswith('queued save') and not (b > a):
        print(f'{name} does not queue save after the mutation')
        ok = False
    if name == 'ship save retry fallback' and not (b < c < d):
        print('ship save retry fallback is missing one of the retry markers')
        ok = False
    if name == 'sql_load_ship SQL authority' and 'redis_' in text_player[sql_load_ship_fn:sql_load_ship_end]:
        print('sql_load_ship still consults or publishes Redis state')
        ok = False
    if name == 'sql_delete_ship redis invalidate' and c == -1:
        print('sql_delete_ship does not invalidate redis snapshot after delete')
        ok = False
    if name == 'rename_ship_owner redis invalidate' and b == -1:
        print('rename_ship_owner does not invalidate redis snapshot for old owner')
        ok = False

if flush_fn == -1:
    print('missing flush_pending_ship_saves helper')
    ok = False

sys.exit(0 if ok else 1)
