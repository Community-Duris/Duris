#!/usr/bin/env python3
"""Real dual-weapon and ordinary mortal spellcast overlap with passive item actions."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess
import tempfile
import time
import run_item_pilot_journey as pilot
import run_weapon_actions_journey as weapon
import run_spellcast_racial_multiplier_journey as caster
import test_flatfile_combat_journey as journey


def run(binary, inspector, dual):
    with tempfile.TemporaryDirectory(prefix='duris-item-overlap-run-') as run_tmp, \
         tempfile.TemporaryDirectory(prefix='duris-item-overlap-state-') as state_tmp:
        root,state=Path(run_tmp),Path(state_tmp); state.chmod(0o700)
        (state/'domains').mkdir(mode=0o700)
        subprocess.run([str(inspector),str(state),'seed-combat'],check=True)
        pilot.configure(root,'studio',True)
        pilot.set_properties(root,{'itemActions.weapons.enabled':1,'itemActions.weapons.windupPulses':24,
            'spellcast.pulse.racial.All':2,'hitpoints.class.Mindflayer':5000})
        obj=root/'areas_mini/mini.obj'; records=pilot.object_records(obj.read_text())
        zone=root/'areas_mini/mini.zon'; text=zone.read_text()
        if dual:
            records[22803]=records[22801].replace('#22801','#22803').replace('wandblade','secondblade')
            text=re.sub(r'(?m)^(M 0 22801 .*\n)',r'\1E 1 22801 1 16 100 0 0 0 * primary\nE 1 22803 1 17 100 0 0 0 * secondary\n',text)
            assert 'E 1 22803' in text,text
            environment={}
        else:
            text=text.replace('\nS\n','\nO 0 22801 1 22800 100 0 0 0 * wielded caster source\nS\n')
            environment={'CHAOS_MUD':'TRUE','CREATION_ALL_CLASSES':'TRUE','CHAOS_STARTER_EPIC_SKILLS':'TRUE'}
        obj.write_text(''.join(records[v] for v in sorted(records))+'$~\n'); zone.write_text(text)
        server=pilot.Server(binary,root,state,'studio',environment)
        try:
            server.start(); client=server.client=journey.MudClient(server.port)
            if dual: journey.create_character(client)
            else: caster.create_mindflayer(client)
            client.send('toggle boon'); client.expect('no longer be affected by boons')
            client.send('drop all'); weapon.drain(client,.7)
            if dual:
                client.send('look sentinel'); equipment=weapon.drain(client,.6)
                assert 'secondblade' in equipment and 'wandblade' in equipment,equipment
            if not dual:
                client.send('get wandblade'); client.expect('You get ')
                client.send('wield wandblade'); client.expect('You wield')
                client.send('toggle quickchant'); client.expect('Quickchant is disabled.')
            client.send('kill sentinel'); started=time.monotonic()
            if dual:
                output=''; deadline=time.monotonic()+90
                while True:
                    output+=weapon.drain(client,.4)
                    if all(f'{name} begins gathering magic toward YOU!' in output and
                           f'{name} releases its gathered magic!' in output for name in ('wandblade','secondblade')): break
                    assert time.monotonic()<deadline,output[-7000:]
                    if 'You stumble' in output[-300:]: client.send('kill sentinel')
                return dict(case='two_native_weapons',both_sources_warned_and_released=True,
                    elapsed_s=round(time.monotonic()-started,3))
            # Start the real mortal mana-based spell after its own weapon has
            # reserved passive work. Quickchant is disabled; no trusted bypass.
            client.expect('wandblade begins gathering magic toward',timeout=90)
            client.send(f"will '{caster.SPELL}'")
            client.expect(caster.START,timeout=8); cast_started=time.monotonic()
            output=client.expect(caster.COMPLETE,timeout=45)
            assert not any(message in output for message in caster.ABORT_MESSAGES),output
            assert 'wandblade releases its gathered magic!' in output,output
            assert 'magic missile' in output,output
            return dict(case='ordinary_mortal_cast_with_own_passive_weapon',normal_cast_completed=True,
                passive_native_spell_released_during_cast=True,cast_seconds=round(time.monotonic()-cast_started,3))
        except Exception:
            print(text)
            if server.client: print(bytes(server.client.transcript).decode(errors='replace').replace(journey.PASSWORD,'[REDACTED]')[-10000:])
            print(journey.runtime_logs(root)); raise
        finally: server.stop()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--inspector',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--case',choices=('all','dual','caster'),default='all')
    args=parser.parse_args(); result=dict(binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),cases=[])
    for dual in (True,False):
        if args.case not in ('all','dual' if dual else 'caster'): continue
        result['cases'].append(run(args.binary.resolve(),args.inspector.resolve(),dual))
        if args.output: args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
