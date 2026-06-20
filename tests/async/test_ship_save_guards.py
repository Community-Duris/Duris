from pathlib import Path
import sys

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

rename_ship = text_base.find('bool rename_ship(P_char ch, char *owner_name, char *new_name)')
rename_guard = text_base.find('if (!write_ship(temp))', rename_ship)
rename_return = text_base.find('return FALSE;', rename_guard)
rename_unlink = text_base.find('unlink(buf);', rename_guard)
checks.append(('rename_ship guard order', rename_ship, rename_guard, rename_return, rename_unlink))

reown = text_base.find('bool rename_ship_owner(char *old_name, char *new_name)')
reown_guard = text_base.find('if (!write_ship(ship))', reown)
reown_return = text_base.find('return FALSE;', reown_guard)
reown_unlink = text_base.find('unlink(buf);', reown_guard)
checks.append(('rename_ship_owner guard order', reown, reown_guard, reown_return, reown_unlink))

dock = text_base.find('ship->anchor = world[to_room].number;')
dock_queue = text_base.find('queue_ship_save(ship, "docking anchor update");', dock)
checks.append(('dock ship queued save', dock, dock_queue, -1, -1))

summon_arrive = text_base.find('ship->speed    = 0;', text_base.find('if (isname(buf, ship->ownername)'))
summon_queue = text_base.find('queue_ship_save(ship, "summon arrival");', summon_arrive)
checks.append(('summon arrival queued save', summon_arrive, summon_queue, -1, -1))

reward = text_combat.find('ship_gain_money(contacts[i].ship, ship, salvage, bounty);')
reward_queue = text_combat.find('queue_ship_save(contacts[i].ship, "combat reward");', reward)
checks.append(('combat reward queued save', reward, reward_queue, -1, -1))

sink = text_combat.find('ship_loss_on_sink(ship, attacker, frag_gain);')
sink_queue = text_combat.find('queue_ship_save(ship, "sink resolution");', sink)
checks.append(('sink queued save', sink, sink_queue, -1, -1))

summon_clear = text_shop.find('clear_cargo(ship);')
summon_clear_queue = text_shop.find('queue_ship_save(ship, "cargo clear during summon");', summon_clear)
checks.append(('summon clear queued save', summon_clear, summon_clear_queue, -1, -1))

sale = text_shop.find('update_crew(ship);')
sale_queue = text_shop.find('queue_ship_save(ship, "cargo sale");', sale)
checks.append(('cargo sale queued save', sale, sale_queue, -1, -1))

repair = text_shop.find('Thank you for for your business, it will take')
repair_queue = text_shop.find('queue_ship_save(ship, "repair");', repair)
checks.append(('repair queued save', repair, repair_queue, -1, -1))

swap = text_shop.find('ship->slot[slot2].clone(temp);')
swap_queue = text_shop.find('queue_ship_save(ship, "slot swap");', swap)
checks.append(('slot swap queued save', swap, swap_queue, -1, -1))

jettison = text_utils.find('update_ship_status(ship);')
jettison_queue = text_utils.find('queue_ship_save(ship, "cargo jettison");', jettison)
checks.append(('cargo jettison queued save', jettison, jettison_queue, -1, -1))

write_ship = text_base.find('int write_ship(P_ship ship)')
retry_pending = text_base.find('ship->save_pending     = true;', write_ship)
retry_delay = text_base.find('ship->save_retry_after = time(NULL) + 1;', retry_pending)
retry_signature = text_base.find('ship->save_saved_signature = ship_save_signature(ship);', write_ship)
flush_fn = text_base.find('void flush_pending_ship_saves(void)')
checks.append(('ship save retry fallback', write_ship, retry_pending, retry_delay, retry_signature))

# ship snapshot redis cache wiring
sql_load_ship_fn = text_player.find('P_ship sql_load_ship(const char *owner_name)')
sql_load_try = text_player.find('redis_load_ship_snapshot', sql_load_ship_fn)
sql_load_query = text_player.find('from ships where owner_name', sql_load_ship_fn)
sql_load_prime = text_player.find('redis_cache_ship_snapshot', sql_load_ship_fn)
checks.append(('sql_load_ship redis flow', sql_load_ship_fn, sql_load_try, sql_load_query, sql_load_prime))

sql_delete_ship_fn = text_player.find('bool sql_delete_ship(const char *owner_name)')
sql_delete_query = text_player.find("delete from ships where owner_name='%s'", sql_delete_ship_fn)
sql_delete_inv = text_player.find('redis_invalidate_ship_snapshot', sql_delete_ship_fn)
checks.append(('sql_delete_ship redis invalidate', sql_delete_ship_fn, sql_delete_query, sql_delete_inv, -1))

reown_inv = text_base.find('redis_invalidate_ship_snapshot(old_name)', reown)
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
    if name == 'sql_load_ship redis flow' and not (b != -1 and c != -1 and d != -1):
        print('sql_load_ship missing one of the redis cache hooks')
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
