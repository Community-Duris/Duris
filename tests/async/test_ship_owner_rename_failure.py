from pathlib import Path

base = Path(__file__).resolve().parents[2]
ship_base = (base / 'src/ships/ship_base.c').read_text()
modify = (base / 'src/modify.c').read_text()
specs = (base / 'src/specs.gellz.c').read_text()

ok = True
checks = {
    'write_ship_fail_rollback': 'if (!write_ship(ship))' in ship_base and 'ship->ownername = old_ownername;' in ship_base and 'name_ship(old_ship_name, ship);' in ship_base,
    'failure_cleanup_free_old_name': 'FREE(failed_ship_name);' in ship_base and 'FREE(failed_ship_keywords);' in ship_base,
    'modify_checks_return': 'if (!rename_ship_owner(old_name, new_name))' in modify and 'Ship ownership update failed.' in modify,
    'specs_checks_return': 'if (!rename_ship_owner(argstring3, skip_spaces(argument)))' in specs,
}
for name, passed in checks.items():
    if not passed:
        print(f'missing expected snippet: {name}')
        ok = False

print('ok' if ok else 'fail')
raise SystemExit(0 if ok else 1)
