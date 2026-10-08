# Drustl's Yerdonia Enslaved: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence raxthan \
  --evidence-format markdown --output docs/reference/zone-story-audits/raxthan.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 42903 | `give=I:42926;receive=C:15000,E:25000;disappear=1` | story: Trosat: strange mushrooms | [areas/qst/raxthan.qst:15](../../../areas/qst/raxthan.qst#L15) |
| 42912 | `give=I:42931;receive=I:42930;disappear=1` | story: Drustl: the enslaver's head | [areas/qst/raxthan.qst:66](../../../areas/qst/raxthan.qst#L66) |
| 42912 | `give=I:42934;receive=I:42938;disappear=1` | story: Drustl: the lost arrow head | [areas/qst/raxthan.qst:78](../../../areas/qst/raxthan.qst#L78) |
| 42915 | `give=I:42957;receive=E:100000,I:42955;disappear=1` | story: Volgk: Agama's tongue | [areas/qst/raxthan.qst:111](../../../areas/qst/raxthan.qst#L111) |
| 42938 | `give=I:42939,I:42940;receive=E:250000,I:42935;disappear=1` | story: Dravkult: head and heart | [areas/qst/raxthan.qst:194](../../../areas/qst/raxthan.qst#L194) |
| 42939 | `give=I:42937;receive=E:50000,I:42936;disappear=1` | story: Rethorn: Morklar's scales | [areas/qst/raxthan.qst:242](../../../areas/qst/raxthan.qst#L242) |
| 42941 | `give=I:42942,I:42949;receive=E:50000,I:42941;disappear=1` | story: Epolon: head and fang | [areas/qst/raxthan.qst:284](../../../areas/qst/raxthan.qst#L284) |
| 42944 | `give=I:42940,I:42948;receive=E:60000,I:42947;disappear=1` | story: T'rin: the two black hearts | [areas/qst/raxthan.qst:329](../../../areas/qst/raxthan.qst#L329) |
| 42945 | `give=I:42956;receive=E:25000,I:42952;disappear=1` | story: Drasklor: Darthus's spine | [areas/qst/raxthan.qst:371](../../../areas/qst/raxthan.qst#L371) |
| 42946 | `give=I:42927,I:42927,I:42927;receive=E:25000,I:42953;disappear=1` | story: Grobklarn: the triad of cave shrooms | [areas/qst/raxthan.qst:398](../../../areas/qst/raxthan.qst#L398) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 42903 | shrooms mushrooms mush smurfs | [areas/qst/raxthan.qst:2](../../../areas/qst/raxthan.qst#L2) |
| 42912 | raxthan quest head enslaver | [areas/qst/raxthan.qst:30](../../../areas/qst/raxthan.qst#L30) |
| 42912 | drusto jasmona son daughter | [areas/qst/raxthan.qst:52](../../../areas/qst/raxthan.qst#L52) |
| 42915 | agama | [areas/qst/raxthan.qst:92](../../../areas/qst/raxthan.qst#L92) |
| 42938 | raxthan | [areas/qst/raxthan.qst:127](../../../areas/qst/raxthan.qst#L127) |
| 42938 | graskal | [areas/qst/raxthan.qst:141](../../../areas/qst/raxthan.qst#L141) |
| 42938 | noctule | [areas/qst/raxthan.qst:174](../../../areas/qst/raxthan.qst#L174) |
| 42939 | morklar scales | [areas/qst/raxthan.qst:222](../../../areas/qst/raxthan.qst#L222) |
| 42941 | elytron | [areas/qst/raxthan.qst:260](../../../areas/qst/raxthan.qst#L260) |
| 42941 | thrask | [areas/qst/raxthan.qst:273](../../../areas/qst/raxthan.qst#L273) |
| 42944 | noctule | [areas/qst/raxthan.qst:304](../../../areas/qst/raxthan.qst#L304) |
| 42944 | bloodbeast | [areas/qst/raxthan.qst:317](../../../areas/qst/raxthan.qst#L317) |
| 42945 | darthus | [areas/qst/raxthan.qst:347](../../../areas/qst/raxthan.qst#L347) |
| 42946 | shroom shrooms | [areas/qst/raxthan.qst:390](../../../areas/qst/raxthan.qst#L390) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

305 parsed reset commands: D: 4, E: 57, F: 4, G: 44, M: 177, O: 9, P: 10.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
