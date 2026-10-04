# Tribal Forest: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence tribal \
  --evidence-format markdown --output docs/reference/zone-story-audits/tribal.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 42200 | `give=I:42201;receive=I:42201;disappear=0` | Excluded: The huge bluebird refuses the larger white-worm meat and returns the same prototype kind without a meaningful reward. Preserve native settlement evidence, but do not count this refusal as a completed story, achievement or daily; same-kind replacement is not proof of identical physical UID. | [areas/qst/tribal.qst:25](../../../areas/qst/tribal.qst#L25) |
| 42200 | `give=I:42202;receive=E:33000,I:42204;disappear=0` | story: The bluebird’s small-meat nest | [areas/qst/tribal.qst:18](../../../areas/qst/tribal.qst#L18) |
| 42200 | `give=I:42265;receive=E:80000,I:42200;disappear=0` | story: The bluebird’s grain and staff | [areas/qst/tribal.qst:11](../../../areas/qst/tribal.qst#L11) |
| 42209 | `give=I:42201;receive=E:50000,I:42265;disappear=0` | story: The hunter’s bluebird grain | [areas/qst/tribal.qst:46](../../../areas/qst/tribal.qst#L46) |
| 42219 | `give=I:42219,I:42220,I:42222,I:42242;receive=I:42268;disappear=1` | story: The wife’s four-part escape | [areas/qst/tribal.qst:77](../../../areas/qst/tribal.qst#L77) |
| 42230 | `give=I:42241;receive=E:250000,I:42260;disappear=0` | story: The young woman’s lost comb | [areas/qst/tribal.qst:119](../../../areas/qst/tribal.qst#L119) |
| 42234 | `give=I:42212,I:42221,I:42227,I:42263,I:42267;receive=I:42262;disappear=0` | story: The shaman’s five-ingredient crystal | [areas/qst/tribal.qst:191](../../../areas/qst/tribal.qst#L191) |
| 42234 | `give=I:42244;receive=I:42224;disappear=0` | story: The shaman’s spotted deerskin | [areas/qst/tribal.qst:180](../../../areas/qst/tribal.qst#L180) |
| 42256 | `give=I:42293,I:42294,I:42295,I:42296,I:42297;receive=I:42285,I:42291;disappear=1` | story: Xazapath’s five scattered parts | [areas/qst/tribal.qst:266](../../../areas/qst/tribal.qst#L266) |
| 42259 | `give=I:42302;receive=E:250000,I:42301;disappear=0` | story: The Spider Queen’s missing egg | [areas/qst/tribal.qst:308](../../../areas/qst/tribal.qst#L308) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 42200 | hi hello forest you going target | [areas/qst/tribal.qst:2](../../../areas/qst/tribal.qst#L2) |
| 42200 | food | [areas/qst/tribal.qst:7](../../../areas/qst/tribal.qst#L7) |
| 42209 | bluebirds birds bird | [areas/qst/tribal.qst:34](../../../areas/qst/tribal.qst#L34) |
| 42219 | hi hello gretings | [areas/qst/tribal.qst:59](../../../areas/qst/tribal.qst#L59) |
| 42219 | husban boss chief chieftain prison imprisonment kitchen | [areas/qst/tribal.qst:64](../../../areas/qst/tribal.qst#L64) |
| 42230 | hi hello greetings | [areas/qst/tribal.qst:98](../../../areas/qst/tribal.qst#L98) |
| 42230 | comb | [areas/qst/tribal.qst:104](../../../areas/qst/tribal.qst#L104) |
| 42234 | hi hello greetings quest | [areas/qst/tribal.qst:134](../../../areas/qst/tribal.qst#L134) |
| 42234 | work details | [areas/qst/tribal.qst:140](../../../areas/qst/tribal.qst#L140) |
| 42234 | another | [areas/qst/tribal.qst:149](../../../areas/qst/tribal.qst#L149) |
| 42256 | hello greeting greetings | [areas/qst/tribal.qst:214](../../../areas/qst/tribal.qst#L214) |
| 42256 | help job work | [areas/qst/tribal.qst:221](../../../areas/qst/tribal.qst#L221) |
| 42256 | reward | [areas/qst/tribal.qst:261](../../../areas/qst/tribal.qst#L261) |
| 42259 | hi hello greetings welcome | [areas/qst/tribal.qst:298](../../../areas/qst/tribal.qst#L298) |
| 42259 | egg eggs missing | [areas/qst/tribal.qst:303](../../../areas/qst/tribal.qst#L303) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 42235 | `amethyst_orb` | [src/specs/specs.assign.c:1738](../../../src/specs/specs.assign.c#L1738) |

## Reset coverage

377 parsed reset commands: D: 38, E: 87, F: 15, G: 30, M: 133, O: 49, P: 25.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
