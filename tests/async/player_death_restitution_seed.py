#!/usr/bin/env python3
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: player_death_restitution_seed.py FULL_PAYLOAD_HEX EMPTY_PAYLOAD_HEX")
payload, second = sys.argv[1:]

print(f"""
INSERT INTO player_death_disposition(pid,save_revision,operation_id,corpse_item_uid,corpse_room_vnum,wallet_revision,wallet_copper,wallet_silver,wallet_gold,wallet_platinum,wallet_pile_uid,payload,recorded_at)
VALUES (42,7,UNHEX('a0a1a2a3a4a5a6a7a8a9aaabacadaeaf'),900,1234,9,1,2,3,4,7000,UNHEX('{payload}'),FROM_UNIXTIME(1700000000)),
       (42,8,UNHEX('b0b1b2b3b4b5b6b7b8b9babbbcbdbebf'),901,1234,9,0,0,0,0,0,UNHEX('{second}'),FROM_UNIXTIME(1700000000));
INSERT INTO player_death_custody(pid,save_revision,item_uid,root_item_uid,parent_item_uid,item_revision,vnum,state,owner_type,owner_id,owner_context_id,owner_revision) VALUES
 (42,7,7000,7000,0,10,3,1,1,42,0,5),
 (42,7,1000,1000,0,10,100,1,1,42,0,5),
 (42,7,1001,1000,1000,10,101,1,1,42,0,5),
 (42,7,1002,1000,1000,10,102,1,1,42,0,5),
 (42,7,1003,1000,1000,10,103,1,1,42,0,5),
 (42,7,1004,1000,1000,10,104,1,1,42,0,5),
 (42,7,1999,1999,0,10,999,1,1,42,0,5),
 (42,8,1003,1003,0,11,103,1,1,42,0,6);
INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES (1,42,0,6);
INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,vnum,item_revision,state) VALUES
 (7000,7000,NULL,1,42,0,3,11,3),
 (1000,1000,NULL,1,42,0,100,11,3),
 (1001,1000,1000,1,42,0,101,11,3),
 (1002,1000,1000,1,42,0,102,11,3),
 (1003,1000,1000,1,42,0,103,11,3),
 (1004,1000,1000,1,42,0,104,11,3);
INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,value7,name,short_descr,description,action_descr,bitvector1,bitvector2,bitvector3,bitvector4,bitvector5,item_material,obj_uid,item_condition)
VALUES (42,300,0,NULL,1,1,5,123,0,0,1,0,0,0,0,0,0,0,0,'newer item','newer item','newer item','newer item',0,0,0,0,0,1,3000,99);
INSERT INTO artifact_domain_state(vnum,owned,loc_type,location,timer_epoch,artifact_type,bind_owner_pid,bind_timer_epoch,item_uid,item_revision,revision) VALUES (104,1,5,42,1700000456,2,42,654321,1004,11,4);
INSERT INTO artifact_bind(vnum,owner_pid,timer) VALUES (104,42,654321);
INSERT INTO artifacts_mortal(vnum,owned,location,timer,type,lastUpdate,locType) VALUES (104,'Y',42,FROM_UNIXTIME(1700000456),2,FROM_UNIXTIME(1700000456),5);
""", end="")
