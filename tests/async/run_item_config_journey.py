#!/usr/bin/env python3
"""Exercise live operator toggles/reload and private aggregate metrics in a disposable server."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time
import run_item_pilot_journey as pilot
import run_weapon_actions_journey as weapon
import test_flatfile_combat_journey as journey

WARNING='A brilliant light gathers along the blade.'
EFFECT='magic missile hits'

def run(binary, inspector):
    with tempfile.TemporaryDirectory(prefix='duris-item-config-run-') as run_tmp, \
         tempfile.TemporaryDirectory(prefix='duris-item-config-state-') as state_tmp:
        root,state=Path(run_tmp),Path(state_tmp); state.chmod(0o700)
        (state/'domains').mkdir(mode=0o700)
        subprocess.run([str(inspector),str(state),'seed-combat'],check=True)
        pilot.configure(root,'studio',True)
        pilot.set_properties(root,{'itemActions.telemetry.enabled':1})
        path=root/'lib/item_abilities.json'; catalog=json.loads(path.read_text())
        ability=catalog['abilities'][0]; ability['cost']=100
        path.write_text(json.dumps(catalog))
        server=pilot.Server(binary,root,state,'studio'); player=None
        results=[]
        try:
            server.start(); operator=server.client=journey.MudClient(server.port)
            journey.create_character(operator)
            operator.send('save'); operator.expect(f'Save complete for {journey.CHARACTER}.')
            server.stop()
            subprocess.run([str(inspector),str(state),'seed-item-operator'],check=True)
            server.start(); operator=server.client=journey.reconnect_character(server.port)
            player=journey.MudClient(server.port)
            journey.create_character(player,account='Dorelin',character='Verenal',email='item-config@example.invalid')
            player.send('toggle boon'); player.expect('no longer be affected by boons')
            player.send('drop all'); weapon.drain(player,.8)
            player.send('get prism'); player.expect('You get ')
            deadline=time.monotonic()+15
            while True:
                player.send('itemmana prism'); output=weapon.drain(player,.5)
                found=re.search(r'Item mana: (\d+)\.(\d{3})',output)
                if found and int(found[1])*1000+int(found[2])>=2000: break
                assert time.monotonic()<deadline,output
            def setting(key,value):
                operator.send(f'properties set {key} {value}')
                output=weapon.drain(operator,.3)
                assert 'Huh?' not in output and 'not found' not in output,output
            def start():
                player.send('use prism sentinel'); player.expect(WARNING,timeout=8)
            def no_effect():
                output=weapon.drain(player,2.5)
                assert EFFECT not in output,output
                return output
            # The same word is an ordinary item lookup for a mortal.
            player.send('itemmana metrics'); output=weapon.drain(player,.4)
            assert 'telemetry:' not in output and 'do not hold' in output,output
            player.send('use prism nobody'); output=weapon.drain(player,.4)
            assert WARNING not in output and EFFECT not in output,output
            results.append('invalid target rejected; mortal metrics hidden')
            for key in ('itemActions.enabled','itemActions.studio.enabled','itemActions.mana.enabled'):
                start(); setting(key,0)
                assert 'gutters out' in no_effect()
                setting(key,1); no_effect()
                results.append(key+': pending cancelled; re-enable did not revive it')
            for key,value,restore in (('itemActions.enabled',2,1),('itemActions.maxPulses',0,120)):
                setting(key,value)
                player.send('use prism sentinel'); output=no_effect()
                assert WARNING not in output,output
                setting(key,restore)
                results.append(key+': invalid numeric value failed closed')
            start()
            ability['revision']=2; ability['mana']['revision']=2
            ability['mana']['capacity']=50000
            path.write_text(json.dumps(catalog))
            operator.send('properties reload'); weapon.drain(operator,.4)
            assert 'gutters out' in no_effect()
            player.send('itemmana prism'); output=weapon.drain(player,1)
            if 'not ready' in output:
                player.send('itemmana prism'); output=weapon.drain(player,1)
            found=re.search(r'Item mana: (\d+)\.(\d{3}) / 50\.000',output)
            assert found,output
            observed=int(found[1])*1000+int(found[2]); assert observed<50000,output
            results.append('revision 2 catalog reload cancelled pending and clamped without refill')
            start(); path.write_text('{ invalid JSON')
            operator.send('properties reload'); operator_output=weapon.drain(operator,.5)
            assert 'gutters out' in no_effect()
            # Last valid definitions survive a malformed catalog; a new action works.
            start(); player.send('abort'); player.expect('gutters out')
            results.append('invalid catalog retained last valid definitions; reload did not revive pending')
            start(); warned=time.monotonic()
            player.expect(EFFECT,timeout=8)
            assert time.monotonic()-warned>=1.65
            operator.send('itemmana metrics'); metrics=weapon.drain(operator,.6)
            assert 'telemetry: enabled' in metrics and 'pending=0' in metrics,metrics
            counts={key:int(value) for key,value in re.findall(r'([a-z_.]+)=(\d+)',metrics)}
            assert counts.get('started',0)>=6 and counts.get('cancelled.reload',0)>=2,metrics
            assert counts.get('completed',0)>=1 and counts.get('callbacks',0)>=1,metrics
            rows=pilot.mana_rows(state); assert len(rows)==1,rows
            setting('itemActions.telemetry.enabled',0)
            operator.send('itemmana metrics'); output=weapon.drain(operator,.4)
            assert 'telemetry: disabled' in output and 'selected=' not in output,output
            return dict(binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                acceptance=results,metrics=counts,reserve_after_profile_reload=observed,
                final_committed_mana=next(iter(rows.values())),telemetry_disable_cleared_window=True)
        except Exception:
            for client in (server.client,player):
                if client: print(bytes(client.transcript).decode(errors='replace').replace(journey.PASSWORD,'[REDACTED]')[-9000:])
            print(journey.runtime_logs(root)); raise
        finally:
            if player: player.close()
            server.stop()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--inspector',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args(); result=run(args.binary.resolve(),args.inspector.resolve())
    if args.output: args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
