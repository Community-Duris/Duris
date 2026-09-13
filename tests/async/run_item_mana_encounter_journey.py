#!/usr/bin/env python3
"""Real-server sustained NPC combat, immediate player encounter and recharge."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import time
import re
import run_item_pilot_journey as pilot
import run_weapon_actions_journey as weapon
import test_flatfile_combat_journey as journey

def reserve(client):
    client.send('itemmana prism')
    text=weapon.drain(client,.3)
    found=re.search(r'Item mana: (\d+)\.(\d{3})',text)
    return int(found[1])*1000+int(found[2]) if found else None

def run(binary,inspector):
    with tempfile.TemporaryDirectory(prefix='duris-mana-encounter-run-') as run_tmp, \
         tempfile.TemporaryDirectory(prefix='duris-mana-encounter-state-') as state_tmp:
        root,state=Path(run_tmp),Path(state_tmp); state.chmod(0o700)
        (state/'domains').mkdir(mode=0o700)
        subprocess.run([str(inspector),str(state),'seed-combat'],check=True)
        pilot.configure(root,'studio',True)
        path=root/'lib/item_abilities.json'; catalog=json.loads(path.read_text())
        ability=catalog['abilities'][0]; ability['cost']=3000
        ability['mana'].update(capacity=10000,regeneration=250)
        path.write_text(json.dumps(catalog))
        server=pilot.Server(binary,root,state,'studio'); enemy=None
        warning='A brilliant light gathers along the blade.'; effect='magic missile hits'
        try:
            server.start(); client=server.client=journey.MudClient(server.port)
            journey.create_character(client)
            client.send('toggle boon'); client.expect('no longer be affected by boons')
            client.send('drop all'); weapon.drain(client,.6)
            client.send('get prism'); client.expect('You get ')
            enemy=journey.MudClient(server.port)
            journey.create_character(enemy,account='Dorelin',character='Verenal',email='item-opponent@example.invalid')
            enemy.send('toggle boon'); enemy.expect('no longer be affected by boons')
            client.send('itemmana prism'); weapon.drain(client,.5)
            deadline=time.monotonic()+50
            while reserve(client)!=10000:
                assert time.monotonic()<deadline,'cold pool did not regenerate to capacity'
            start=time.monotonic(); casts=0; attempts=0; outcomes=[]
            while time.monotonic()-start<60:
                client.send('stand'); weapon.drain(client,.25)
                before=reserve(client); client.send('use prism sentinel'); attempts+=1
                output=weapon.drain(client,2.6)
                started=warning in output; completed=effect in output
                if completed: casts+=1
                outcomes.append(dict(attempt=attempts,starting_units=before,started=started,completed=completed))
            # Finish at a low reserve, making the immediate player attempt
            # unambiguously unaffordable even across one wall-clock second.
            while True:
                before=reserve(client)
                if before is not None and before<2000: break
                if before is not None and before>=3000:
                    client.send('stand'); weapon.drain(client,.25)
                    client.send('use prism sentinel'); output=weapon.drain(client,2.6)
                    attempts+=1; completed=effect in output; casts+=int(completed)
                    outcomes.append(dict(attempt=attempts,starting_units=before,started=warning in output,completed=completed))
            immediate=reserve(client); assert immediate<2250,immediate
            client.send('use prism verenal'); refused=weapon.drain(client,.6)
            assert warning not in refused and effect not in refused,refused
            recovered_at=time.monotonic()
            while True:
                recovered=reserve(client)
                if recovered is not None and recovered>=3000: break
                assert time.monotonic()-recovered_at<15,'recharge deadline exceeded'
            recharge_s=time.monotonic()-recovered_at
            enemy.pending.clear(); client.send('use prism verenal')
            client.expect(warning,timeout=5); started=time.monotonic()
            output=client.expect(effect,timeout=8)
            victim=enemy.expect('magic missile',timeout=8)
            row=next(iter(pilot.mana_rows(state).values()))
            result=dict(binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                profile=dict(capacity=10000,cost=3000,regeneration_per_second=250),
                initial_units=10000,pve_attempts=attempts,pve_completed=casts,pve_outcomes=outcomes,
                immediate_player_encounter_units=immediate,immediate_player_ability_suppressed=True,
                recharge_seconds=round(recharge_s,3),recovered_units=recovered,
                player_spell_after_recharge=True,warning_to_player_damage_s=round(time.monotonic()-started,3),
                final_committed_mana=row)
            assert casts>1 and casts<attempts,result
            return result
        except Exception:
            for connection in (server.client,enemy):
                if connection:
                    print(bytes(connection.transcript).decode(errors='replace').replace(journey.PASSWORD,'[REDACTED]')[-9000:])
            print(journey.runtime_logs(root)); raise
        finally:
            if enemy: enemy.close()
            server.stop()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--inspector',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args(); result=run(args.binary.resolve(),args.inspector.resolve())
    if args.output: args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
