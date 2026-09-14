#!/usr/bin/env python3
"""Three real Telnet players; hold MariaDB item acknowledgements while looter moves.

Requires a freshly built MariaDB server and TEST_DB_HOST/USER/PASSWORD pointing
to a disposable loopback database. Creates and drops its own unique schema.
"""
from pathlib import Path
import os
import signal
import subprocess
import sys
import tempfile
import time
import uuid
import test_flatfile_combat_journey as journey

ROOT=Path(__file__).resolve().parents[2]

def drain(client, duration=.5):
    until=time.monotonic()+duration
    while time.monotonic()<until:
        client._receive()
        time.sleep(.02)
    text=client.pending.decode(errors='replace')
    client.pending.clear()
    return text

def reconnect_linkdead(port):
    client=journey.MudClient(port)
    entry,_=client.expect_any(('term type','account name'))
    if entry=='term type': client.send('9'); client.expect('account name')
    client.send(journey.ACCOUNT); client.expect('enter your password')
    client.send(journey.PASSWORD); client.expect('PRESS RETURN'); client.send('')
    client.expect('Please select an option'); client.send('1'); client.expect(journey.CHARACTER)
    client.send('1'); outcome,_=client.expect_any(('Reconnecting...','Play as'))
    if outcome=='Play as': client.send('y'); client.expect('The Regression Arena')
    return client

def run(binary):
    database='haul_journey_'+uuid.uuid4().hex[:12]
    host=os.environ['TEST_DB_HOST']
    assert host in ('localhost','127.0.0.1')
    env=dict(PATH=os.environ.get('PATH','/usr/bin:/bin'), ENVIRONMENT='local',
             DB_HOST=host,DB_PORT='3306',DB_NAME=database,DB_USER=os.environ['TEST_DB_USER'],
             DB_PASSWD=os.environ['TEST_DB_PASSWORD'],MYSQL_PWD=os.environ['TEST_DB_PASSWORD'],
             DB_ALLOWED_TARGETS=host+'/'+database,PERSISTENCE_MODE='mariadb-primary',DB_TLS='FALSE',
             REDIS='FALSE',CHAOS_MUD='FALSE',LISTEN_ADDRESS='127.0.0.1',
             DURIS_WEBSOCKET_LISTEN_ADDRESS='127.0.0.1')
    mysql=['mysql','--protocol=tcp','-h',host,'-u',env['DB_USER'],'-N','-B','--unbuffered']
    def sql(statement, selected=True):
        return subprocess.check_output(mysql+([database] if selected else []),input=statement,
                                       text=True,env=env).strip()
    sql('CREATE DATABASE '+database,False)
    try:
        sql((ROOT/'migrations/bootstrap_multithread_safe.sql').read_text())
        for args in [('adopt','--kind','fresh_bootstrap'),('run',)]:
            subprocess.run(['python3','scripts/migration_runner.py',*args],cwd=ROOT,env=env,check=True)
        with tempfile.TemporaryDirectory(prefix='corpse-haul-',dir=ROOT/'bin/tests') as temporary:
            runtime=Path(temporary)
            journey.make_fixture(runtime,True); journey.generate_certificate(runtime)
            wld=runtime/'areas_mini/mini.wld'; world=wld.read_text()
            world=world.replace('This quiet stone arena exists to prove the complete combat journey.\n~\n1 0 0\nS',
                'This quiet stone arena exists to prove the complete combat journey.\n~\n1 0 0\nD0\n~\n~\n0 -1 22801\nS')
            world=world.replace('$~','#22801\nThe Observer Landing~\nAn adjacent room for the observer.\n~\n1 0 0\nD2\n~\n~\n0 -1 22800\nS\n$~')
            assert '0 -1 22801' in world
            wld.write_text(world)
            (runtime/'logs/log').mkdir(parents=True)
            for name in ('players','critical'): (runtime/'journals'/name).mkdir(parents=True,mode=0o700)
            port,tls,ws=journey.available_ports()
            env.update(PLAYER_SAVE_JOURNAL_DIR=str(runtime/'journals/players'),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(runtime/'journals/critical'),
                       DURIS_TLS_PORT=str(tls),DURIS_WEBSOCKET_PORT=str(ws))
            clients=[]; process=None; lock=None
            output=(runtime/'server.out').open('w')
            try:
                process=subprocess.Popen([str(binary),'--minimal','-s','-d',str(runtime),str(port)],
                                         cwd=runtime,env=env,stdout=output,stderr=subprocess.STDOUT)
                deadline=time.monotonic()+120
                while 'Entering game loop.' not in (runtime/'server.out').read_text(errors='replace'):
                    assert process.poll() is None and time.monotonic()<deadline,'boot failed'
                    time.sleep(.1)
                actor=journey.MudClient(port); clients.append(actor); journey.create_character(actor)
                source=journey.MudClient(port); clients.append(source)
                journey.create_character(source,account='Sourceacct',character='Sourcemortal',email='source@example.invalid')
                dest=journey.MudClient(port); clients.append(dest)
                journey.create_character(dest,account='Destacct',character='Destmortal',email='dest@example.invalid')
                dest.send('north'); dest.expect('The Observer Landing')
                actor.send('toggle boon'); actor.expect('You will no longer be affected by boons.')
                actor.send('wield mace'); actor.expect('You wield')
                actor.send('drop all'); actor.expect('You drop a steel long sword.',timeout=20)
                actor.send('kill raoul')
                deadline=time.monotonic()+45
                while True:
                    outcome,_=actor.expect_any(('is dead! R.I.P.','You stumble, but recover in time!',
                       'You stumble in your attack, and jab at','You stumble in your attack, and hit yourself!'),
                       timeout=max(1,deadline-time.monotonic()))
                    if outcome=='is dead! R.I.P.': break
                    assert time.monotonic()<deadline
                    actor.send('kill raoul')
                actor.send('look in corpse'); actor.expect('banana')
                actor.send('save'); actor.expect('Save complete for Taverek.',timeout=30)
                pid=int(sql("SELECT pid FROM player_data WHERE name='Taverek'"))

                def held_departure(label, target='corpse', name='the corpse of Raoul'):
                    nonlocal lock
                    for c in clients: drain(c)
                    lock=subprocess.Popen(mysql+[database],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE,text=True,env=env,bufsize=1)
                    lock.stdin.write("LOCK TABLES item_current_owner WRITE; SELECT 'held';\n"); lock.stdin.flush()
                    assert lock.stdout.readline().strip()=='held'
                    actor.send('get all '+target); actor.expect('You begin pulling things from',timeout=20)
                    before=drain(actor)
                    assert 'Haul:' not in before and 'You get ' not in before,before
                    actor.send('north'); actor.expect('The Observer Landing',timeout=15)
                    # Observe room output while the durable acknowledgement is held.
                    source_before=drain(source); dest_before=drain(dest)
                    assert 'begins pulling things from' in source_before,source_before
                    assert 'gets ' not in dest_before and 'pulling things' not in dest_before,dest_before
                    lock.stdin.write('UNLOCK TABLES;\n'); lock.stdin.flush(); lock.stdin.close()
                    assert lock.wait(timeout=15)==0; lock=None
                    actor.expect('You finish sorting your haul from '+name,timeout=30)
                    final=drain(actor,1)
                    assert 'Haul:' in final,final
                    destination=drain(dest)
                    assert 'gets ' not in destination and 'pulling things' not in destination,destination
                    assert 'You get ' not in final,final
                    print(label+': '+final,flush=True)
                    actor.send('south'); actor.expect('The Regression Arena')
                    return final

                # First missing-stock acknowledgement only adopts the banana at
                # its source; departure prevents a new ownership transfer.
                first=held_departure('stock adoption')
                assert 'Nothing acquired.' in first and 'no longer available' in first,first
                assert sql(f'SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND vnum=15')=='0'
                second=held_departure('equipment transfer')
                assert 'a banana' in second and 'remaining contents' in second,second
                assert sql(f'SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND vnum=15 AND state=1')=='1'
                actor.send('inventory'); actor.expect('a banana')
                before_wallet=sql(f'SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}')
                third=held_departure('coin adoption')
                assert 'Nothing acquired.' in third,third
                assert sql(f'SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}')==before_wallet
                fourth=held_departure('coin transfer')
                assert '3s' in fourth and 'Nothing acquired' not in fourth,fourth
                assert sql(f'SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}')=='0\t3\t0\t0'
                actor.send('save'); actor.expect('Save complete for Taverek.',timeout=30)
                actor.close(); clients.remove(actor)
                # Reconnect to the same live player, verify inventory and save.
                actor=reconnect_linkdead(port); clients.append(actor)
                actor.send('inventory'); actor.expect('a banana')
                actor.send('save'); actor.expect('Save complete for Taverek.',timeout=30)
                assert 'finish sorting' not in drain(actor), 'completion replayed on reconnect'
                # Repeat the accepted item and coin boundary against the actual
                # player's corpse, including its lifecycle revision publication.
                journey.attack_until_death(actor); actor.expect('ACCOUNT MENU',timeout=45)
                actor.close(); clients.remove(actor)
                actor=journey.reconnect_character(port); clients.append(actor)
                actor.send('look'); actor.expect('The corpse of a Human is lying here.')
                pc_items=held_departure('player corpse equipment','Taverek','the corpse of Taverek')
                assert 'a banana' in pc_items and 'remaining contents' in pc_items,pc_items
                pc_coins=held_departure('player corpse coins','Taverek','the corpse of Taverek')
                assert '3s' in pc_coins,pc_coins
                actor.send('save'); actor.expect('Save complete for Taverek.',timeout=30)
                assert sql(f'SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}')=='0\t3\t0\t0'
                print('PASS: actual three-player NPC/player corpse held SQL adoption/item/coin movement, observers, durable custody, wallet and reconnect',flush=True)
            except Exception:
                print((runtime/'server.out').read_text(errors='replace')[-5000:])
                print(journey.runtime_logs(runtime)[-12000:])
                for c in clients: print(c.transcript.decode(errors='replace')[-7000:])
                raise
            finally:
                if lock and lock.poll() is None: lock.kill(); lock.wait()
                for c in clients: c.close()
                if process and process.poll() is None:
                    process.send_signal(signal.SIGTERM); process.wait(timeout=30)
                output.close()
    finally: sql('DROP DATABASE '+database,False)

if __name__=='__main__': run(Path(sys.argv[1]).resolve())
