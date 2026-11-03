# Father Tel's Holy Cloister: source-only boundaries and typed-reference supplement

Status: repository-wide static supplement to the existing
[Cloister dossier](../zone-stories/FATHER_TELS_HOLY_CLOISTER.md). It reconciles
the active WLD boundaries, current unregistered WLD inputs, direct external
typed references, and the active-title/history boundary. No zone source, story
sidecar, or gameplay code was changed or qualified.

## Active and source-only WLD boundaries

Cloister has 157 authored and loader-resolving WLD exits, two local weak
components (largest: 39 rooms), no fall or current fields, and two active
reciprocal boundary pairs:

| Active boundary | Evidence |
| --- | --- |
| Cloister room 67100 north ↔ Surface room 537902 south | [Cloister exit](../../../areas/wld/cloister.wld#L13), [Surface exit](../../../areas/wld/surface.wld#L795692) |
| Cloister room 67138 north ↔ Clan Stoutdorf room 34351 south | [Cloister exit](../../../areas/wld/cloister.wld#L732), [Stoutdorf exit](../../../areas/wld/drst.wld#L3220) |

The active [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) records
two inbound and two outbound directed edges, all structurally paired. The
existing dossier documents the secret/key flags and access semantics; reciprocal
source records alone do not qualify a usable route.

I parsed the authored D0–D9 records in all 95 current unregistered WLD files:
2,080,773 exits, with zero parse issues. Two source-only exits target the
active Cloister room set:

| Unregistered source record | Direction and active target | Evidence |
| --- | --- | --- |
| `surf.wld` room 229643 | South (`D2`) to Cloister room 67100 | [exit target](../../../areas/wld/surf.wld#L621846) |
| `underworld2.wld` room 417516 | North (`D0`) to Cloister room 67100 | [exit target](../../../areas/wld/underworld2.wld#L10352) |

These exits are authored in unregistered WLD files and are not active-world
entrances. They do not change the 157-exit active census or prove ordinary
access, safe arrival, or a return path. They add two individually reviewed
records to the curated source-only list, bringing its count from 367 to 369.

## Direct external typed references

The existing dossier maps the local contracts, Alatorin ring supply, Jade and
Winterhaven egg consumers, imported bodies, procedure-driven switch/trap
behavior, and competing source encounters. This bounded exact-reference scan
records the following direct typed uses of Cloister prototypes:

| Current source | Cloister prototype and role | Evidence |
| --- | --- | --- |
| Alatorin reset | Reset group declares local elder ring 67113 | [reset row](../../../areas/zon/alatorin.zon#L2603), [ring](../../../areas/obj/cloister.obj#L186) |
| Jade quest | Saint Macavor's request consumes Troggahn's egg 67116 with head 76730 | [Jade QST](../../../areas/qst/jade.qst#L273), [Jade story mapping](../../../areas/story/jade.story.json#L1423), [egg](../../../areas/obj/cloister.obj#L234) |
| Winterhaven quest | Lancer's request consumes egg 67116 for gift 55420, 250,000 nominal XP, and 350,000 copper | [Winterhaven QST](../../../areas/qst/wh.qst#L2618), [Winterhaven story mapping](../../../areas/story/wh.story.json#L1019) |
| Chaos equipment tables | Class equipment-kit entries reference local glowing jade earring 67104 | [kit entry](../../../src/account/chaos_eq_data.h#L75), [earring](../../../areas/obj/cloister.obj#L69) |
| Epic-teacher code | The table maps Father Tel mobile 67103 to `SKILL_ADVANCED_MEDITATION`; epic initialization binds the shared teacher procedure from that table | [teacher table](../../../src/classes/epic_skills.c#L170), [binding](../../../src/world/epic.c#L1372), [Father Tel](../../../areas/mob/cloister.mob#L51) |

The compiled epic-teacher binding is independent of the prototype's
`ACT_TEACHER` flag. That flag is not computed for Cloister's local mobiles;
Father Tel's separate VNUM entry in the epic-teacher table still exists. The
existing dossier's reset-generated objects, source bodies, and two competing
foreign egg requests remain separately mapped. These declarations do not prove
live stock, successful request completion, kit selection, actor availability,
or a player's receipt.

## Active title and tracked history boundary

The normalized active title “Father Tel's Holy Cloister” has one owner: area
`cloister`, zone 671. The reviewed tracked WLD/ZON path history and active
manifest name no distinct predecessor or replacement. This finding is limited
to title and file-history evidence; Checkpoint 711 compares the unregistered Underworld2 HOLY CLOISTER room and the Father Tel references in Tharnadia and Surface QST text. These are a source-only connector lead and in-world lore references rather than competing active area titles; no separate predecessor or replacement is indicated. See [Checkpoint 711](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-711-cloister-title-and-source-only-name-comparison).

[Active title](../../../areas/zon/cloister.zon#L2) ·
[active-area entry](../../../areas/AREA#L253) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Audit boundary

These are static source findings. Active boundary geometry and typed records do
not qualify secret searches, keys, doors, trap state, source availability,
foreign delivery, item custody, or the orphan room's return access in play.
Editor/tooling references, persisted-player state, and indirect or dynamic
consumers remain outside this bounded closure.
