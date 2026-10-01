#!/usr/bin/env python3
"""Check supported physical and pouch craft admission before live mutation."""
from _paths import extract_function

movement = extract_function('item_movement_transaction.c',
                            'bool item_movement_transaction_submit_craft(')
admission = movement.index('economic_gameplay_authority::prepare_item_transfer(')
assert admission < movement.index('pending.emplace(')
assert admission < movement.index('critical_command_coordinator_submit_for_publication(')
assert 'economic_source_kind::crafting' in movement[admission:]

for symbol in ('do_mixpoison', 'do_encrust'):
    function = extract_function('salchemist.c', 'void ' + symbol + '(')
    assert 'item_movement_transaction_submit_craft(' in function
    assert 'economic_gameplay_authority::active()' not in function
encrust = extract_function('salchemist.c', 'void do_encrust(')
assert encrust.index('chaos_material_pouch_can_record_generated') < encrust.index('number(1, 110)')
assert 'virtual_jewel ? 1 : 2' in encrust
assert 'virtual_jewel ? &pouch_usage : nullptr' in encrust
assert 'chaos_material_pouch_record_generated(' not in encrust
harvester = extract_function('drannak.c', 'int pvp_store(')
assert 'item_movement_transaction_submit_craft(' in harvester
assert 'economic_gameplay_authority::active()' not in harvester
print('supported alchemy admission and atomic virtual-material craft passed')
