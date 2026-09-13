#!/usr/bin/env python3
"""Disposable native/Studio activation, durable debit, interruption and restart."""
from __future__ import annotations
import argparse
from contextlib import ExitStack
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import shutil
import struct
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey
import run_weapon_actions_journey as weapon

ROOT = Path(__file__).resolve().parents[2]


def set_properties(root, values):
    path = root/'lib/duris.properties'
    text = path.read_text()
    for key, value in values.items():
        text, count = re.subn(rf'(?m)^{re.escape(key)}=.*$', f'{key}={value}', text)
        if not count:
            text += f'\n{key}={value}\n'
    path.write_text(text)


def object_records(text):
    starts = list(re.finditer(r'^#(\d+)\s*\n', text, re.M))
    return {int(match[1]): text[match.start():starts[i+1].start() if i+1<len(starts) else text.index('$~')]
            for i, match in enumerate(starts)}


def native_template(vnum):
    inventory = json.loads((ROOT/'docs/reference/artifact_source_inventory.json').read_text())
    entry = next(row for row in inventory['templates'] if row['vnum']==vnum)
    text = (ROOT/entry['source']).read_text(encoding='latin1')
    return object_records(text+'\n$~')[vnum]


def configure(root, family, enabled):
    weapon.configure(root, False)
    set_properties(root, {'itemActions.enabled': int(enabled), 'itemActions.mana.enabled': 1,
        'itemActions.studio.enabled': 1, 'hitpoints.class.Warrior': 5000, 'artifact.respawn': 2})
    set_properties(root,{f'mines.{key}':0 for key in
        ('maxSurfaceMap','maxTharnRift','maxUD','maxGemSurface','maxGemUD')})
    vnum = {'tsunami':31514,'ioun':922,'studio':22802}[family]
    if family!='studio':
        set_properties(root, {f'itemActions.artifact.{vnum}.{key}':value for key,value in
            dict(enabled=1,manaCost=2000,manaCapacity=100000,manaRegen=1000,passiveFloor=0).items()})
        source = native_template(vnum)
        lines=source.split('~',4)[4].strip().splitlines()
        header=list(map(int,lines[0].split())); values=list(map(int,lines[1].split()))
        base=list(map(int,lines[2].split())); bits=(base[3:]+[0]*5)[:5]
        affects=[int(value) for pair in re.findall(r'^A\s*\n(-?\d+) (-?\d+)',source,re.M) for value in pair]
        fields=[vnum,header[0],header[1],header[4],*header[6:11],*base[:3],*values,*bits,*(affects+[0]*8)[:8]]
        (root/'pilot.item').write_text(' '.join(map(str,fields))+'\n')
    else:
        source = '''#22802
regression prism~
the regression prism~
A regression prism lies here.~
~
3 6 3 8 7 0 0 16385 0 0 0
12 99 99 32 0 0 0 0
2 0 100
'''
        catalog = json.loads((ROOT/'docs/examples/studio-item-abilities.json').read_text())
        ability = catalog['abilities'][1]
        ability.update(cost=2000, cooldownMs=0, source='carried')
        ability['mana'].update(capacity=100000, regeneration=1000, passiveFloor=0)
        (root/'lib/item_abilities.json').write_text(json.dumps({'schemaVersion':1,'abilities':[ability]}))
    path = root/'areas_mini/mini.obj'
    records = object_records(path.read_text()); records[vnum]=source
    path.write_text(''.join(records[v] for v in sorted(records))+'$~\n')
    zone = root/'areas_mini/mini.zon'
    text = zone.read_text() if family=='ioun' else re.sub(r'^E .*\n', '', zone.read_text(), flags=re.M)
    if family=='tsunami':
        text=text.replace('\nS\n','\nM 0 22800 1 22800 100 0 0 0 * corpse journey\nS\n')
    zone.write_text(text.replace('\nS\n', f'\nO 0 {vnum} 1 22800 100 0 0 0 * pilot\nS\n') if family=='studio' else text)
    # Area-debugger boot against isolated world files includes Studio and the
    # normal durable artifact restoration. Only this temporary symlink is removed.
    (root/'areas').unlink()
    (root/'areas').mkdir()
    generated=('world.wld','world.mob','world.obj','world.zon','world.trg',
               'world.shp','world.qst','world.tab','world.weather')
    for path in (ROOT/'areas').iterdir():
        if path.name not in generated:
            (root/'areas'/path.name).symlink_to(path,target_is_directory=path.is_dir())
    for suffix in ('wld','mob','obj','zon'):
        (root/f'areas/world.{suffix}').write_text((root/f'areas_mini/mini.{suffix}').read_text())
    for name in ('world.shp','world.qst','world.tab','world.weather'):
        fixture=root/'areas_mini'/name
        (root/'areas'/name).write_text(fixture.read_text() if fixture.exists() else '$~\n')
    (root/'areas/world.trg').write_text('#22802 O\nT CMD use\nitemability 1002\n~\nS\n#~\n' if family=='studio' else '#~\n')


def mana_rows(state):
    rows = {}
    for path in (state/'domains').glob('artifact-mana-*'):
        data = path.read_bytes()
        assert len(data)==104 and data[:8]==b'DURMANA\x01', path
        assert hashlib.sha256(data[:72]).digest()==data[72:], path
        values = struct.unpack('<8Q',data[8:72])
        rows[values[0]] = dict(zip(('uid','profile','profile_revision','version','capacity','regeneration','reserve','settled'),values))
    return rows


class Server:
    def __init__(self,binary,root,state,family,environment=None):
        self.binary,self.root,self.state,self.family=binary,root,state,family
        self.environment=environment or {}
        self.port,self.tls,self.ws=journey.available_ports(); self.process=None; self.client=None
        for name in ('players','critical'):
            (root/'journals'/name).mkdir(parents=True,mode=0o700)
    def start(self):
        env={'PATH':os.environ.get('PATH','/usr/bin:/bin'),'ENVIRONMENT':'local',
            'PERSISTENCE_MODE':'flatfile-primary','FLATFILE_STATE_DIR':str(self.state),
            'PLAYER_SAVE_JOURNAL_DIR':str(self.root/'journals/players'),
            'CRITICAL_COMMAND_JOURNAL_DIR':str(self.root/'journals/critical'),
            'LISTEN_ADDRESS':'127.0.0.1','DURIS_TLS_PORT':str(self.tls),
            'DURIS_WEBSOCKET_LISTEN_ADDRESS':'127.0.0.1','DURIS_WEBSOCKET_PORT':str(self.ws),
            'REDIS':'FALSE','CHAOS_MUD':'FALSE'}
        if 'LD_LIBRARY_PATH' in os.environ: env['LD_LIBRARY_PATH']=os.environ['LD_LIBRARY_PATH']
        env.update(self.environment)
        self.output_path=self.root/'server.out'; self.output=self.output_path.open('w')
        self.process=subprocess.Popen([str(self.binary),'-z','-d',str(self.root),str(self.port)],
            cwd=self.root,env=env,stdout=self.output,stderr=subprocess.STDOUT)
        deadline=time.monotonic()+60
        while 'Entering game loop.' not in self.output_path.read_text(errors='replace'):
            assert self.process.poll() is None and time.monotonic()<deadline, self.output_path.read_text()[-7000:]
            time.sleep(.1)
    def stop(self):
        if self.client: self.client.close(); self.client=None
        if self.process and self.process.poll() is None:
            self.process.send_signal(signal.SIGTERM)
            try: self.process.wait(timeout=30)
            except subprocess.TimeoutExpired: self.process.kill(); self.process.wait(timeout=5)
        if self.process: self.output.close()


def run_case(binary,inspector,family,enabled,abort):
    ending='crash' if abort=='crash' else 'abort' if abort else 'complete'
    case=f'{family}-{"on" if enabled else "off"}-{ending}'
    print('pilot journey: '+case,flush=True)
    with ExitStack() as stack:
        root=Path(stack.enter_context(tempfile.TemporaryDirectory(prefix='duris-item-pilot-run-')))
        state=Path(stack.enter_context(tempfile.TemporaryDirectory(prefix='duris-item-pilot-state-')))
        state.chmod(0o700); (state/'domains').mkdir(mode=0o700)
        subprocess.run([str(inspector),str(state),'seed-combat'],check=True)
        configure(root,family,enabled)
        server=Server(binary,root,state,family)
        try:
            server.start(); client=server.client=journey.MudClient(server.port)
            journey.create_character(client,class_name='c' if family=='tsunami' else 'w')
            client.send('toggle boon'); client.expect('no longer be affected by boons')
            client.send('drop all'); weapon.drain(client,.5)
            name={'tsunami':'trident','ioun':'mirrored','studio':'prism'}[family]
            if family!='studio':
                client.send('save'); client.expect(f'Save complete for {journey.CHARACTER}.')
                server.stop()
                subprocess.run([str(inspector),str(state),'seed-item',str(root/'pilot.item')],check=True)
                server.start(); client=server.client=journey.reconnect_character(server.port)
            else:
                client.send('get '+name); client.expect('You get ')
            if family=='ioun':
                client.send('wear mirrored'); client.expect('begins orbiting your head')
                client.send('kill sentinel')
                started=time.monotonic()
                observed=client.expect('flashes and deflects the incoming spell',timeout=180)
                assert 'Your a mirrored ioun stone' in observed or 'Your mirrored ioun stone' in observed,observed
                elapsed=time.monotonic()-started
                weapon.drain(client,.6); records=mana_rows(state)
                assert len(records)==1,records
                row=next(iter(records.values())); assert row['uid']==1000922 and row['version']>=2,row
                result=dict(case=case,attack_to_paid_interception_s=round(elapsed,3),committed_mana=row,
                    real_native_incoming_spell=True,real_native_deflection=True)
                print(json.dumps(result),flush=True); return result
            if family=='tsunami': client.send('wield '+name); client.expect('You wield')
            command='tap trident' if family=='tsunami' else 'use prism sentinel'
            warning='Water begins gathering' if family=='tsunami' else 'A brilliant light gathers along the blade.'
            effect='You feel vitalized.' if family=='tsunami' else 'magic missile hits'
            weapon.drain(client,.3)
            if enabled:
                client.send(command) # Cold enrollment rejects without paying or starting a cooldown.
                initial=weapon.drain(client,.5); assert warning not in initial,initial
                deadline=time.monotonic()+20
                while True:
                    client.send('itemmana '+name); observed=weapon.drain(client,.5)
                    value=re.search(r'Item mana: (\d+)\.(\d{3})',observed)
                    if value and int(value[1])*1000+int(value[2])>=7000: break
                    assert time.monotonic()<deadline,observed
                before=mana_rows(state); assert len(before)==1,before
            started=time.monotonic(); client.send(command)
            if not enabled:
                observed=client.expect(effect,timeout=8); assert warning not in observed
                return dict(case=case,command_to_effect_s=round(time.monotonic()-started,3))
            client.expect(warning,timeout=8); warned=time.monotonic()
            if abort=='crash':
                weapon.drain(client,.4)
                paid=mana_rows(state); uid=next(iter(before))
                assert paid[uid]['version']>before[uid]['version'],paid
                server.process.kill(); server.process.wait(timeout=10)
                server.stop(); server.start()
                client=server.client=journey.reconnect_character(server.port)
                observed=weapon.drain(client,3)
                assert effect not in observed and warning not in observed,observed
                assert mana_rows(state)==paid,(paid,mana_rows(state))
                elapsed=time.monotonic()-warned
            elif abort:
                client.send('abort'); observed=client.expect('disperses' if family=='tsunami' else 'gutters out')
                elapsed=time.monotonic()-warned
                observed+=weapon.drain(client,3); assert effect not in observed,observed
            else:
                observed=client.expect(effect,timeout=8); elapsed=time.monotonic()-warned
                assert elapsed>=1.65,(elapsed,observed)
            weapon.drain(client,.5); after=mana_rows(state); assert after.keys()==before.keys()
            uid=next(iter(before)); old,new=before[uid],after[uid]
            expected=min(old['capacity'],old['reserve']+(new['settled']-old['settled'])*old['regeneration'])-2000
            assert new['reserve']==expected and new['version']>old['version'],(old,new,expected)
            result=dict(case=case,warning_to_result_s=round(elapsed,3),effect=not abort,
                uid=uid,committed_mana_before=old,committed_mana_after=new,cost_units=2000)
            if family=='studio' and abort!='crash':
                client.send('get bag'); client.expect('You get ',timeout=10)
                # A completed offensive spell starts ordinary combat. Leave it
                # before using the normal container commands.
                if not abort:
                    client.send('flee')
                    while True:
                        escaped,_=client.expect_any(('The Regression Refuge','PANIC!','You scramble madly to your feet!'),timeout=8)
                        if escaped=='The Regression Refuge': break
                        client.send('flee')
                else:
                    client.send('north'); client.expect('The Regression Refuge')
                # Area-debugger mode deliberately skips restoring floor custody.
                # Use the empty refuge for new post-restart room transfers.
                client.send('put prism bag'); client.expect('Ok.',timeout=10)
            client.send('save'); client.expect(f'Save complete for {journey.CHARACTER}.')
            server.stop(); stopped=mana_rows(state)
            server.start(); client=server.client=journey.reconnect_character(server.port,
                expected_room='The Regression Refuge' if family=='studio' and abort!='crash' else 'The Regression Arena')
            if family=='studio' and abort!='crash':
                client.send('get prism bag'); client.expect('You get ',timeout=10)
                client.send('drop prism'); client.expect('You drop ',timeout=10)
                client.send('get prism'); client.expect('You get ',timeout=10)
                result['storage_and_transfer']={'nested_bag_restart':True,'room_drop_get':True}
            if family=='tsunami':
                # Native profiles are published lazily by their callback. The
                # persisted five-minute tap cooldown must reject this attempt.
                client.send(command); retry=weapon.drain(client,.5)
                assert warning not in retry,retry
            deadline=time.monotonic()+15
            while True:
                client.send('itemmana '+name); observed=weapon.drain(client,.5)
                value=re.search(r'Item mana: (\d+)\.(\d{3})',observed)
                if value: break
                assert time.monotonic()<deadline,observed
            reserve=int(value[1])*1000+int(value[2]); current=mana_rows(state)
            assert uid in current and current[uid]==stopped[uid],(stopped,current)
            maximum=min(new['capacity'],new['reserve']+(int(time.time())-new['settled'])*new['regeneration'])
            assert new['reserve']<=reserve<=maximum and reserve<new['capacity'],(new,reserve,maximum)
            assert warning not in observed,observed
            result['restart']={'same_uid':True,'durable_record_unchanged':True,'observed_reserve_units':reserve,
                'maximum_from_elapsed_regeneration':maximum,'pending_revived':False}
            if family=='tsunami' and not abort:
                before_death=mana_rows(state)
                journey.attack_until_death(client); client.expect('ACCOUNT MENU',timeout=45)
                client.close(); client=server.client=journey.reconnect_character(server.port,'You rejoin the land of the living')
                client.send(f'get trident {journey.CHARACTER}'); client.expect('You get ',timeout=15)
                client.send('save'); client.expect(f'Save complete for {journey.CHARACTER}.')
                authority=json.loads(subprocess.check_output([str(inspector),str(state),'inspect','1'],text=True))
                assert any(item['uid']==uid and item['vnum']==31514 for item in authority['player_items']),authority
                assert mana_rows(state)==before_death,(before_death,mana_rows(state))
                result['corpse_loot']={'same_uid':True,'durable_mana_unchanged':True,'real_player_death_and_loot':True}
            print(json.dumps(result),flush=True)
            return result
        except Exception:
            if server.client:
                print(bytes(server.client.transcript).decode(errors='replace').replace(journey.PASSWORD,'[REDACTED]')[-10000:],flush=True)
            print(journey.runtime_logs(root),flush=True)
            if os.environ.get('DURIS_KEEP_ITEM_FAILURE')=='1':
                server.stop()
                kept=Path(tempfile.mkdtemp(prefix='pilot-failure-',dir=ROOT/'bin'))
                shutil.copytree(root,kept/'runtime',symlinks=True)
                shutil.copytree(state,kept/'state',symlinks=True)
                print('retained disposable failure: '+str(kept),flush=True)
            raise
        finally: server.stop()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--inspector',type=Path,default=journey.INSPECTOR)
    parser.add_argument('--family',choices=('tsunami','studio','ioun'),default='tsunami')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--case',choices=('all','off-complete','on-complete','on-abort','on-crash'),default='all')
    args=parser.parse_args()
    cases=((False,False),(True,False),(True,True)) if args.family=='tsunami' else ((True,False),) if args.family=='ioun' else ((True,False),(True,True),(True,'crash'))
    cases=[case for case in cases if args.case=='all' or args.case==f'{"on" if case[0] else "off"}-{"crash" if case[1]=="crash" else "abort" if case[1] else "complete"}']
    if not cases: parser.error('this family does not support the requested case')
    result=dict(binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),cases=[])
    if args.output: args.output.write_text(json.dumps(result,indent=2)+'\n')
    for case in cases:
        result['cases'].append(run_case(args.binary.resolve(),args.inspector.resolve(),args.family,*case))
        if args.output: args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
