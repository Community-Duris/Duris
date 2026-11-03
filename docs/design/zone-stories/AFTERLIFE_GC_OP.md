# Afterlife GC OP

Canonical key: `component:afterlife_op`. Physical coverage row 355. This is a
three-room source-only portal passage with no local mobile prototypes, QST, or
shop file. Its [technical audit](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md)
records the external reset owners, portal target values, active Torg collision,
and separate `void_gh` room overlap. The [builder review](AFTERLIFE_GC_OP_BUILDER_QUESTIONS.md)
keeps destination and ownership decisions open.

## A passage between places

The white-stone passage is engraved with a spaced “AFTERLIFE” inscription.
One room frames a large shimmering portal as a route from the lands of the
good races toward the dark jungles of Khomani-Kan. A second portal object is
described as leading to the Goodie Continent. The circular chamber and tunnel
mouth make the three-room map read as an arrival, junction, and onward passage.

Those destination names are authored descriptions, not verified routes. The
two local type-25 portal objects encode target VNUMs #29106 and #29110. Both
IDs have competing WLD owners, and their encoded destinations do not match
the prose labels. The source exit from #29112 also points upward to #157174,
“The Ocean,” in the unlisted `Duris3.wld`. None of these source records proves
current access or a safe return.

## Rooms and source connections

| Room | Source title | Authored environment and local connection |
| --- | --- | --- |
| #29110 | A Portal Room | White-stone dead end with a large shimmering portal engraved as a passage toward Khomani-Kan. East (`D1`) leads to #29111. The ZON resets local object #29110 here. |
| #29111 | A Large Circular Stone Chamber | Carved chamber with a raised center and a raised white-wood walkway toward the south and west tunnels. West (`D3`) returns to #29110; south (`D2`) leads to #29112. |
| #29112 | Before a Smooth Stone Tunnel | A stone stair and tunnel entrance bears the spaced “AFTERLIFE” engraving; the tunnel is too deep to see through. North (`D0`) returns to #29111; up (`D4`) targets #157174 in `Duris3.wld`. Four mobiles are reset here from the separate `afterlife_gh` MOB source. |

The four local WLD edges form one weak component, and directed traversal from
#29110 reaches all three source rooms. The external D4 edge is the only WLD
boundary exit in this component. The target room has no direct return edge to
#29112 and is not listed in the full `areas/AREA` input.

## Objects and reset ownership

| Object | Description in source | Encoded target | Reset/source owner |
| --- | --- | --- | --- |
| #29110 | “A glowing portal” described as ready to transport its user to Khomani-Kan. | Type 25, `value[0]` #29106; `value[1]` 7. | Reset once in #29110; prototype is local to `afterlife_op.obj`. |
| #29111 | “A glowing portal” described as ready to transport its user to the Goodie Continent. | Type 25, `value[0]` #29110; `value[1]` 7. | Prototype is local to `afterlife_op.obj`; `afterlife_gh.zon` imports it into #29106. |
| #11053 | Player Castles prototype “a magic mouth.” | No portal target inferred from its item description. | Reset in #29112 from `player_castles.obj`. |
| MOB #29101–#29104 | No local mobile descriptions exist in this empty MOB file. | Not applicable. | Reset in #29112; matching definitions occur in `afterlife_gh.mob` and competing unregistered MOB sources. |

The source has no `afterlife_op.qst` or `.shp`. Its `.mob` file exists as an
empty file and declares no prototypes. It therefore contains environmental
story and portal objects without a local quest dialogue or named cast.
`areas/map/afterlife_op.json` preserves editor positions for all three rooms;
it is a separate editor asset, not proof of full-loader membership.

The [audit](../quest-story-followups/AFTERLIFE_OP_AUDIT_REVIEW.md#typed-resets-and-portal-targets)
links every reset and owner record. Do not treat the portal descriptions,
numeric targets, or editor layout as confirmation that the zones are loaded,
that a character can use the items, or that a return route exists.
