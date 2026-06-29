from pathlib import Path

base = Path(__file__).resolve().parents[2]
ship_base = (base / 'src/ships/ship_base.c').read_text()
modify = (base / 'src/modify.c').read_text()
specs = (base / 'src/specs.gellz.c').read_text()

ok = True
checks = {
    'write_ship_failure_is_logged': 'if (!write_ship(ship))' in ship_base and 'Failed to save re-owned ship' in ship_base and 'redis_invalidate_ship_snapshot(old_name);' in ship_base,
    'no_write_ship_rollback': 'ship->ownername = old_ownername;' in ship_base and 'name_ship(old_ship_name, ship);' in ship_base,
    'modify_checks_return': 'if (!rename_ship_owner(old_name, new_name))' in modify and 'Ship ownership update failed.' in modify,
    'specs_checks_return': 'if (!rename_ship_owner(argstring3, skip_spaces(argument)))' in specs,
    'delete_ship_aborts_on_db_failure': 'Failed to delete ship row for %s; aborting ship removal.' in ship_base and 'if (!sql_delete_ship(ship->ownername))' in ship_base,
}
for name, passed in checks.items():
    if not passed:
        print(f'missing expected snippet: {name}')
        ok = False

print('ok' if ok else 'fail')
raise SystemExit(0 if ok else 1)
