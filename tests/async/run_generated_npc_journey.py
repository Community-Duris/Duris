#!/usr/bin/env python3
"""Actual player strings two generated-template instances and verifies repeated copyover.

This exercises recovery through real commands, sockets, save and exec. The NPCs
are controlled instances of 1255/1256, not a full-world random-generation test.
"""
from pathlib import Path
import os, re, shutil, subprocess, sys, tempfile, time, uuid
import test_flatfile_combat_journey as journey

FIXTURE = '\n#include "flatfile/flatfile_player_snapshot_file.h"\n#include "flatfile/flatfile_store.h"\n#include "player/player_snapshot_codec.h"\n#include <cassert>\n#include <openssl/sha.h>\ntemplate<class T> void number(std::vector<uint8_t>& out, T value) {\n    for (size_t i=0; i<sizeof(T); ++i) out.push_back(static_cast<uint64_t>(value) >> (i*8));\n}\nint main(int argc, char **argv) {\n    assert(argc == 2);\n    const std::string root = argv[1];\n    for (uint32_t pid : {1}) {\n        player_snapshot snapshot; std::string error;\n        assert(flatfile_player_snapshot_read(root,pid,&snapshot,&error) == flatfile_player_load_result::ok);\n        for (auto &field : snapshot.status_integers) {\n            if (field.field == player_status_field::level || field.field == player_status_field::highest_level)\n                field.signed_value = field.unsigned_value = 62;\n            if (field.field == player_status_field::base_hit) field.signed_value = field.unsigned_value = 200000;\n            if (field.field == player_status_field::hit_difference) field.signed_value = field.unsigned_value = 0;\n        }\n        std::vector<uint8_t> payload, bytes;\n        snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;\n        assert(player_snapshot_encode(snapshot,&payload) == player_snapshot_codec_result::ok);\n        using namespace flatfile_player_snapshot_file;\n        bytes.insert(bytes.end(),player_magic.begin(),player_magic.end());\n        number(bytes,player_file_version); number<uint32_t>(bytes,payload.size());\n        number(bytes,snapshot.pid); number(bytes,snapshot.revision); number(bytes,snapshot.components);\n        unsigned char digest[SHA256_DIGEST_LENGTH]; SHA256(payload.data(),payload.size(),digest);\n        bytes.insert(bytes.end(),digest,digest+sizeof(digest)); bytes.insert(bytes.end(),payload.begin(),payload.end());\n        assert(flatfile_atomic_write(player_directory(root),player_filename(pid),bytes,&error));\n    }\n}\n'

def drain(client,duration=.4):
    end=time.monotonic()+duration
    while time.monotonic()<end: client._receive(); time.sleep(.02)
    text=client.pending.decode(errors='replace'); client.pending.clear(); return text

def run(binary, mode='file'):
    assert mode in ('file', 'redis')
    with tempfile.TemporaryDirectory(prefix='generated-npc-journey-') as temporary:
        root=Path(temporary); state=root/'state'; runtime=root/'runtime'
        state.mkdir(mode=0o700); (state/'domains').mkdir(mode=0o700); runtime.mkdir()
        (runtime/'logs/log').mkdir(parents=True)
        journey.make_fixture(runtime); journey.generate_certificate(runtime)
        # The deployed prototypes use generic placeholder strings. Real string
        # commands below give each instance independently owned identity text.
        path=runtime/'areas_mini/mini.mob'; mobs=path.read_text()
        for vnum,key in [(1255,'maptemplate'),(1256,'themetemplate')]:
            mob=f"#{vnum}\n{key}~\nrandom mob prototype~\nRandom mob prototype stands here.\n~\n~\n10 0 0 0 0 0 0 0 S\nPH 0 0 -1\n35 0 -30 40d1+900 6d5+12\n0.0.0.0 0\n8 8 0\n"
            mobs, replaced = re.subn(rf'(?ms)^#{vnum}\n.*?(?=^#\d+\n|^\$~)', lambda _: mob, mobs)
            assert replaced == 1, (vnum, replaced)
        path.write_text(mobs)
        path=runtime/'areas_mini/mini.zon'
        path.write_text(path.read_text().replace('\nS\n','\nM 0 1255 1 22800 100 0 0 0\nM 0 1256 1 22800 100 0 0 0\nS\n'))
        path=runtime/'areas_mini/mini.wld'
        world=path.read_text().replace('1 0 0\nS\n$~', '1 0 0\nD0\n~\n~\n0 0 22801\nS\n$~')
        assert '0 0 22801' in world
        path.write_text(world.replace('$~', '#22801\nThe Observation Room~\nA quiet room north of the sentinels.\n~\n1 0 0\nD2\n~\n~\n0 0 22800\nS\n$~'))
        for name in ('players','critical'): (runtime/'journals'/name).mkdir(parents=True,mode=0o700)
        (runtime/'bin/server').mkdir(parents=True)
        for name in ('dms','dms_new'): shutil.copy2(binary,runtime/'bin/server'/name)
        port,tls,ws=journey.available_ports()
        env=dict(PATH=os.environ.get('PATH','/usr/bin:/bin'),ENVIRONMENT='local',PERSISTENCE_MODE='flatfile-primary',
                 FLATFILE_STATE_DIR=str(state),PLAYER_SAVE_JOURNAL_DIR=str(runtime/'journals/players'),
                 CRITICAL_COMMAND_JOURNAL_DIR=str(runtime/'journals/critical'),REDIS='FALSE',CHAOS_MUD='FALSE',
                 LISTEN_ADDRESS='127.0.0.1',DURIS_TLS_PORT=str(tls),DURIS_WEBSOCKET_PORT=str(ws),
                 DURIS_WEBSOCKET_LISTEN_ADDRESS='127.0.0.1')
        database = None
        if mode == 'redis':
            database = 'generated_npc_'+uuid.uuid4().hex[:12]
            host = os.environ['TEST_DB_HOST']; assert host in ('127.0.0.1', 'localhost')
            env.update(DB_HOST=host, DB_PORT='3306', DB_NAME=database, DB_USER=os.environ['TEST_DB_USER'],
                DB_PASSWD=os.environ['TEST_DB_PASSWORD'], MYSQL_PWD=os.environ['TEST_DB_PASSWORD'],
                DB_ALLOWED_TARGETS=host+'/'+database, DB_TLS='FALSE', PERSISTENCE_MODE='mariadb-primary')
            mysql=['mysql','--protocol=tcp','-h',host,'-u',env['DB_USER'],'-N','-B']
            def sql(statement, selected=True):
                return subprocess.check_output(mysql+([database] if selected else []), input=statement,
                    text=True, env=env).strip()
            sql('CREATE DATABASE '+database, False)
            sql((journey.ROOT/'migrations/bootstrap_multithread_safe.sql').read_text())
            for arguments in [('adopt','--kind','fresh_bootstrap'),('run',)]:
                subprocess.run(['python3','scripts/migration_runner.py',*arguments],cwd=journey.ROOT,env=env,check=True)
        else:
            subprocess.run([str(journey.INSPECTOR),str(state),'seed-combat'],check=True)
        cpp=root/'staff.cpp'; cpp.write_text(FIXTURE); fixture=root/'staff'
        subprocess.run(['g++','-std=c++20','-Isrc',str(cpp),'src/player/player_snapshot_codec.c',
            'src/flatfile/flatfile_player_snapshot_file.c','src/flatfile/flatfile_store.c','-lcrypto','-o',str(fixture)],cwd=journey.ROOT,check=True)
        process=output=client=redis=None
        def stop():
            nonlocal client,process,output
            if client: client.close(); client=None
            if process and process.poll() is None: process.terminate(); process.wait(timeout=30)
            if output: output.close()
        def boot():
            nonlocal process,output
            output=(runtime/'server.out').open('w')
            process=subprocess.Popen([str(runtime/'bin/server/dms'),'--minimal','-s',str(port)],cwd=runtime,env=env,stdout=output,stderr=subprocess.STDOUT)
            deadline=time.monotonic()+90
            while 'Entering game loop.' not in (runtime/'server.out').read_text(errors='replace'):
                assert process.poll() is None and time.monotonic()<deadline,'boot failed'
                time.sleep(.1)
        try:
            boot(); client=journey.MudClient(port); journey.create_character(client)
            client.send('save'); client.expect('Save complete for Taverek.')
            client.send('quit'); client.expect('ACCOUNT MENU',timeout=30)
            stop()
            if mode == 'file': subprocess.run([str(fixture),str(state)],check=True)
            else: sql("UPDATE player_data SET level=62,highest_level=62,base_hit=200000 WHERE name='Taverek'")
            if mode == 'redis':
                redis_port = journey.available_ports()[0]
                redis = subprocess.Popen(['redis-server', '--bind', '127.0.0.1', '--port', str(redis_port),
                    '--save', '', '--appendonly', 'no'], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
                env.update(REDIS='TRUE', REDIS_HOST='127.0.0.1', REDIS_PORT=str(redis_port),
                    REDIS_NAMESPACE='duris:local:generated-npc-journey',
                    REDIS_WORLD_STATE='TRUE', REDIS_WORLD_STATE_INTERVAL='5',
                    REDIS_WORLD_STATE_SECRET='local-generated-npc-fixture-secret-123456789')
                time.sleep(.3)
            boot(); client=journey.reconnect_character(port); drain(client)
            for key,name,description in [('maptemplate','mapward','Mapward the wandering sentinel'),
                                         ('themetemplate','themeward','Themeward the cavern sentinel')]:
                for field,value,target in [('name',name,key),('short',description,name),('long',description+' stands watch.',name)]:
                    client.send(f'string char {target} {field} {value}'); client.expect('Ok.')
                client.send(f'setattr {name} str 103'); print('setattr: '+drain(client),flush=True)
            initial={}
            for cycle in range(3):
                drain(client); client.send('look'); text=client.expect('Pos: standing >')
                assert text.count('Mapward the wandering sentinel stands watch.')==1,text
                assert text.count('Themeward the cavern sentinel stands watch.')==1,text
                assert 'random mob prototype' not in text.lower(),text
                for name in ('mapward','themeward'):
                    client.send('stat mob '+name); stat=drain(client,1)
                    assert '103' in stat,stat
                    signature=(re.search(r'Numbers:.*?(\d+)-I', stat).group(1),
                        re.search(r'Level:.*',stat).group(0), re.search(r'Race:.*',stat).group(0),
                        tuple(re.findall(r'(?:Str|Dex|Agi|Con|Kar|Pow|Int|Wis|Cha|Luc):\s*\d+\s*\(\s*(\d+)\)',stat)),
                        tuple(re.findall(r'[PGSC]coins:\s*(\d+)',stat)))
                    if cycle == 0: initial[name]=signature
                    else:
                        expected=initial[name] if mode=='file' else initial[name][:-1]+(tuple('0' for _ in signature[-1]),)
                        assert signature == expected, (name, expected, signature)
                    print(f'{cycle} {name}: '+stat,flush=True)
                    if '[Return to continue' in stat:
                        client.send('q'); drain(client)
                client.send('north'); client.expect('The Observation Room'); drain(client)
                client.send('scan'); scanned=drain(client,1)
                assert 'Mapward the wandering sentinel' in scanned and 'Themeward the cavern sentinel' in scanned,scanned
                assert 'random mob prototype' not in scanned.lower(),scanned
                client.send('south'); client.expect('The Regression Arena'); drain(client)
                if cycle<2:
                    client.send('save'); client.expect('Save complete for Taverek.',timeout=30)
                    if mode == 'file':
                        client.send('shutdown copyover'); client.expect('Copyover complete!',timeout=90)
                        client.send('save'); client.expect('Save complete for Taverek.',timeout=30)
                    else:
                        deadline = time.monotonic() + 60
                        while 'generation and floor handoff acknowledged' not in journey.runtime_logs(runtime):
                            assert time.monotonic() < deadline, 'no acknowledged Redis snapshot'
                            time.sleep(.2)
                        if cycle == 0:
                            process.kill(); process.wait(timeout=30)
                        else:
                            client.send('shutdown reboot'); process.wait(timeout=30)
                        stop(); boot(); client=journey.reconnect_character(port); drain(client)
                        assert 'restored world recovery generation' in journey.runtime_logs(runtime)
            print(f'PASS: real player string/stat/look, {mode} repeated recovery, retained 1255/1256 identities/base strength/counts and save',flush=True)
        except Exception:
            print((runtime/'server.out').read_text(errors='replace')[-6000:])
            print(journey.runtime_logs(runtime)[-12000:])
            if client: print(client.transcript.decode(errors='replace')[-9000:])
            raise
        finally:
            stop()
            if redis:
                redis.terminate(); redis.wait(timeout=15)
            if database: sql('DROP DATABASE '+database, False)

if __name__=='__main__': run(Path(sys.argv[1]).resolve(), sys.argv[2] if len(sys.argv)>2 else 'file')
