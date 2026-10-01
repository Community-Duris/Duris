#!/usr/bin/env python3
"""Execute active alchemy entry refusals and check allocation boundaries."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

alchemy = (ROOT/'src/classes/salchemist.c').read_text()
movement = extract_function('item_movement_transaction.c',
                            'bool item_movement_transaction_submit_craft(')
assert movement.index('economic_gameplay_authority::active()') < movement.index('player_item_snapshot_tree_capture')
harvester = extract_function('drannak.c', 'int pvp_store(')
buy = harvester[harvester.index('else if (strstr(arg, "1"))'):]
assert buy.index('economic_gameplay_authority::active()') < buy.index('vnum_in_inv')
assert buy.index('economic_gameplay_authority::active()') < buy.index('read_object')
code = r'''
#include <cassert>
struct character { int ingredients=3, jewels=1; };
using P_char=character *;
struct economic_gameplay_authority { static bool active() { return true; } };
static int notices=0, reached=0;
void send_to_char(const char *, P_char) { ++notices; }
'''
for symbol, tail in [('do_mixpoison','\n\tP_obj vial;'), ('do_encrust','\n\tchar arg[')]:
    function = extract_function('salchemist.c', 'void '+symbol+'(')
    prefix = function[:function.index(tail)]
    assert prefix.index('economic_gameplay_authority::active()') < prefix.index('send_to_char')
    code += prefix.replace('char *argument', 'char * /*argument*/') + '\n++reached;\n}\n'
code += r'''
int main() {
    character player; char arguments[]="mace green";
    do_mixpoison(&player,arguments,0); do_encrust(&player,arguments,0);
    assert(notices==2 && reached==0 && player.ingredients==3 && player.jewels==1);
}
'''
with tempfile.TemporaryDirectory(prefix='active-alchemy-refusal-') as directory:
    path=Path(directory);(path/'test.cpp').write_text(code)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('active poison/Encrust entry refusals and Harvester/craft allocation boundaries passed')
