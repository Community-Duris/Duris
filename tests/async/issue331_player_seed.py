#!/usr/bin/env python3
"""Emit the private issue331 disputed/quarantined SQL fixture.

The fixture is intentionally parameterized by the real pid created through the
player account flow.  It seeds only the disposable database selected by the
caller; it never connects to SQL itself and never prints credentials.
"""
from __future__ import annotations

import re
import sys
import time


UIDS = (51000, 51001, 51002, 51003, 51006, 51005)
OPERATION = "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
TIMER = int(time.time()) + 604800


def number(value: str, label: str, minimum: int = 1) -> int:
    if not re.fullmatch(r"[0-9]+", value):
        raise SystemExit(f"invalid {label}")
    parsed = int(value)
    if parsed < minimum:
        raise SystemExit(f"invalid {label}")
    return parsed


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: issue331_player_seed.py PID PAYLOAD_HEX OWNER_REVISION")
    pid = number(sys.argv[1], "pid")
    owner_revision = number(sys.argv[3], "owner revision")
    payload = sys.argv[2].lower()
    if len(payload) % 2 or not re.fullmatch(r"[0-9a-f]+", payload):
        raise SystemExit("invalid payload")
    print(f"""
INSERT INTO player_death_disposition
 (pid,save_revision,operation_id,corpse_item_uid,corpse_room_vnum,
  wallet_revision,wallet_copper,wallet_silver,wallet_gold,wallet_platinum,
  wallet_pile_uid,payload)
VALUES
 ({pid},77,UNHEX('{OPERATION}'),50999,22800,9,1,2,3,4,51006,UNHEX('{payload}'));

INSERT INTO player_death_custody
 (pid,save_revision,item_uid,root_item_uid,parent_item_uid,item_revision,vnum,state,
  owner_type,owner_id,owner_context_id,owner_revision)
VALUES
 ({pid},77,51000,51000,0,10,391,1,1,{pid},0,{owner_revision}),
 ({pid},77,51001,51000,51000,10,15,1,1,{pid},0,{owner_revision}),
 ({pid},77,51002,51002,0,10,677,1,1,{pid},0,{owner_revision}),
 ({pid},77,51003,51003,0,10,67259,1,1,{pid},0,{owner_revision}),
 ({pid},77,51006,51006,0,10,3,1,1,{pid},0,{owner_revision}),
 ({pid},77,51005,51005,0,10,7,1,1,{pid},0,{owner_revision});

INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision)
VALUES (1,{pid},0,{owner_revision + 1})
ON DUPLICATE KEY UPDATE revision=VALUES(revision);

INSERT INTO item_current_owner
 (item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,
  vnum,item_revision,state)
VALUES
 (51000,51000,NULL,1,{pid},0,391,11,3),
 (51001,51000,51000,1,{pid},0,15,11,3),
 (51002,51002,NULL,1,{pid},0,677,11,3),
 (51003,51003,NULL,1,{pid},0,67259,11,3),
 (51006,51006,NULL,1,{pid},0,3,11,3),
 (51005,51005,NULL,1,{pid},0,7,11,3);

INSERT INTO item_ownership_quarantine
 (item_uid,source_table,source_row_id,conflict_code,evidence)
VALUES
 (51000,'player_death_custody',51000,331,'issue331 disputed death custody'),
 (51001,'player_death_custody',51001,331,'issue331 disputed nested custody'),
 (51002,'player_death_custody',51002,331,'issue331 disputed weapon custody'),
 (51003,'player_death_custody',51003,331,'issue331 disputed unique artifact custody'),
 (51006,'player_death_custody',51006,331,'issue331 disputed currency custody'),
 (51005,'player_death_custody',51005,331,'issue331 custody-only item');

INSERT INTO artifact_domain_state
 (vnum,owned,loc_type,location,timer_epoch,artifact_type,bind_owner_pid,
  bind_timer_epoch,item_uid,item_revision,revision)
VALUES
 (67259,1,5,{pid},{TIMER},2,{pid},{TIMER},51003,11,4);
INSERT INTO artifact_bind(vnum,owner_pid,timer)
VALUES (67259,{pid},{TIMER});
INSERT INTO artifacts_mortal(vnum,owned,location,timer,type,lastUpdate,locType)
VALUES (67259,'Y',{pid},FROM_UNIXTIME({TIMER}),2,FROM_UNIXTIME({TIMER}),5);
""", end="")


if __name__ == "__main__":
    main()
