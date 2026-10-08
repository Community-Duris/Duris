# The Arcaneum of L'srillizzin: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence library \
  --evidence-format markdown --output docs/reference/zone-story-audits/library.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 402000 | `give=C:10000;receive=I:402003;disappear=0` | service: Gafrizzen: admission key | [areas/qst/library.qst:10](../../../areas/qst/library.qst#L10) |
| 402010 | `give=I:402004,I:402005,I:402006,I:402007,I:402008,I:402009;receive=E:10000;disappear=0` | story: Yralrife: six elemental essences | [areas/qst/library.qst:21](../../../areas/qst/library.qst#L21) |
| 402018 | `give=I:402020,I:402022,I:402023,I:402024,I:402026,I:402027,I:402028;receive=E:100000;disappear=0` | story: Rikn: seven undead proofs | [areas/qst/library.qst:37](../../../areas/qst/library.qst#L37) |
| 402027 | `give=I:402029;receive=C:50000;disappear=0` | story: Gringobbi: the historical journal | [areas/qst/library.qst:72](../../../areas/qst/library.qst#L72) |
| 402037 | `give=I:402041,I:402042,I:402043,I:402044,I:402045,I:402046,I:402047,I:402048;receive=I:402049;disappear=1` | story: Gazdiel: eight distinct dreams | [areas/qst/library.qst:85](../../../areas/qst/library.qst#L85) |
| 402061 | `give=I:402011,I:402012,I:402015,I:402016,I:402017,I:402018,I:402019,I:402050,I:402051,I:402067;receive=I:402089;disappear=1` | story: Razeline: ten apprentice books | [areas/qst/library.qst:137](../../../areas/qst/library.qst#L137) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 402000 | student | [areas/qst/library.qst:2](../../../areas/qst/library.qst#L2) |
| 402000 | scholar | [areas/qst/library.qst:6](../../../areas/qst/library.qst#L6) |
| 402010 | summoning essences | [areas/qst/library.qst:17](../../../areas/qst/library.qst#L17) |
| 402018 | necromantic necromancy necro arts | [areas/qst/library.qst:33](../../../areas/qst/library.qst#L33) |
| 402026 | cleric clerical teacher | [areas/qst/library.qst:50](../../../areas/qst/library.qst#L50) |
| 402027 | thief assassin rogue | [areas/qst/library.qst:56](../../../areas/qst/library.qst#L56) |
| 402027 | mercenary brigand bounty hunter | [areas/qst/library.qst:60](../../../areas/qst/library.qst#L60) |
| 402027 | job | [areas/qst/library.qst:64](../../../areas/qst/library.qst#L64) |
| 402037 | fragment dream dreams memories | [areas/qst/library.qst:79](../../../areas/qst/library.qst#L79) |
| 402041 | curse | [areas/qst/library.qst:103](../../../areas/qst/library.qst#L103) |
| 402058 | warrior | [areas/qst/library.qst:115](../../../areas/qst/library.qst#L115) |
| 402059 | antipaladin anti-paladin | [areas/qst/library.qst:121](../../../areas/qst/library.qst#L121) |
| 402060 | monk teacher specialize | [areas/qst/library.qst:127](../../../areas/qst/library.qst#L127) |
| 402061 | books | [areas/qst/library.qst:133](../../../areas/qst/library.qst#L133) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

366 parsed reset commands: D: 86, E: 27, G: 42, M: 153, O: 46, P: 12.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
