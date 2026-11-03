# Duke Delwyn typed-dependency audit

Checkpoint 785 records a bounded static typed-reference review for active
Delwyn. It preserves the current source and does not change native gameplay.

## Current source inventory

Delwyn is an active full-area contributor with 207 rooms (#82800–#83006),
96 mobiles (#82800–#82895), 41 objects (#82800–#82840), and 36 QST givers.
The active QST source contains 53 native blocks: eleven Q contracts and 42
responses. The area has no local SHP file. Its story sidecar contains 58 typed
VNUM fields: 19 mob_vnum entries, 28 giver_vnum references, and eleven
item_vnums entries. Their typed targets resolve to local mobiles, QST givers,
and objects respectively; all 28 completion keys contain 54 item-VNUM
references (15 unique), and each resolves to a local object prototype.

The ZON contains 278 commands: 135 M, 67 E, 38 D, 28 O, five F, four G, and
one P. Its 235 M/E/O/G/P prototype-reset commands and 236 typed prototype
fields, including the P container, resolve to local prototypes. Reset
declarations do not establish runtime availability.

## External typed consumers

A full current active and source-only scan found no external QST item or giver
reference, SHP product, or structured story-sidecar VNUM reference to a
Delwyn prototype. No active ZON prototype reset imports a Delwyn mobile or
object, and no external mobile or object definition duplicates a Delwyn
prototype VNUM. The bounded C/C++ scan found no direct numeric reference to a
Delwyn room, mobile, or object.

### Candidate room reset overlap

Unregistered candidate Nexus56 has six D reset room fields and seven M
placement-room fields in its ZON using #83000–#83006. Nexus56 also defines
those seven VNUMs in its own WLD, with room titles distinct from active
Delwyn. Preserve these as source-only, candidate-local reset locations on the
existing Nexus56 row; the overlapping numbers do not establish active Delwyn
resets or prototype imports. The existing Nexus56 WLD direction from its
room #83007 west to Delwyn #83006 remains the separately documented
source-only approach; it is already included in the global route total.

## Item travel, shops, and disposition

Delwyn has no local type-25 ITEM_TELEPORT or type-29 ITEM_SWITCH object.
Across the other active and source-only object files, the scan covered 872
type-25 declarations and 380 type-29 declarations; none targets a Delwyn
room. No outside SHP product targets a Delwyn object. No local or inbound
portal/switch room field was found.

Keep active Delwyn **DO NOT MOVE**. Preserve the reciprocal Surface gate,
the already-counted Nexus56 source-only approach, and the candidate-local
reset locations with the existing Nexus56 **RETAIN** disposition. No new
candidate row or disposition change; existing candidate row 78 now records the
reset-room overlap. No WLD exit total changes, and no runtime access, reset
supply, shop operation, or safe route is inferred.
Duke Delwyn’s existing 17 owner questions remain unchanged.
