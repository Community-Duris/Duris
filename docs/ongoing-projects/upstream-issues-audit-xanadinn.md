# Upstream Origin Issues Audit (`xanadinn/DurisMUD`)

A technical audit of all 8 issues filed in upstream repository [`https://github.com/xanadinn/DurisMUD/issues`](https://github.com/xanadinn/DurisMUD/issues), evaluated against the current state of the Duris codebase.

---

## Executive Summary Matrix

| # | Upstream Issue Title | Upstream State | Current Codebase Status | Category |
|---|---|:---:|:---:|---|
| **#1** | [DEFINITIONS (Read First) OPIs/balance/zone issues and suggested solutions](https://github.com/xanadinn/DurisMUD/issues/1) | `OPEN` | **OUTSTANDING** | Content / Item Balance Design Proposals |
| **#3** | [Hellfire vs. Paladin](https://github.com/xanadinn/DurisMUD/issues/3) | `OPEN` | **OUTSTANDING** | Combat & Spell Mechanics Balance |
| **#4** | [Minotaur Balance](https://github.com/xanadinn/DurisMUD/issues/4) | `OPEN` | **OUTSTANDING** | Racial Stat / Innate Balance |
| **#5** | [3rd Racewar Side Re-Enabling](https://github.com/xanadinn/DurisMUD/issues/5) | `OPEN` | **OUTSTANDING** | Game Architecture / Feature Request |
| **#6** | [single hand 2 hand weapons (race/class)](https://github.com/xanadinn/DurisMUD/issues/6) | `OPEN` | **OUTSTANDING** | Equipment / Combat Mechanics Request |
| **#7** | [Case senistive areas](https://github.com/xanadinn/DurisMUD/issues/7) | `OPEN` | **PARTIALLY MITIGATED** | Build Tooling & Asset Auditing |

---

## Detailed Findings per Issue

### Issue #1: OPIs / Balance / Zone Issues & Suggested Solutions
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/1`](https://github.com/xanadinn/DurisMUD/issues/1)
- **Status in Current Codebase**: **OUTSTANDING (Unimplemented Proposals)**
- **Technical Analysis**:
  This issue contains long-form balance proposals targeting three specific items and general mechanics:
  1. **`the mace of mentality` (`VNUM 44188`)**:
     - *Mechanism*: Procs `spell_reflection(50)` on a 5-minute timer via `CMD_SAY "mentality"` ([`src/specs.object.c:14231`](file:///home/aiwithapex/projects/duris/src/specs.object.c#L14231)).
     - *Spawn Location*: Still configured as a quest reward in Tikitzopl ([`areas/qst/tikitt.qst:255`](file:///home/aiwithapex/projects/duris/areas/qst/tikitt.qst#L255)).
     - *Proposal*: Convert to a Winterhaven Quest Item (WHQI). Currently unchanged.
  2. **`a metal flask marked 'word of recall'` / `a scroll of word of recall`**:
     - *Mechanism*: Portable `spell_word_of_recall` ([`src/magic.c:8288`](file:///home/aiwithapex/projects/duris/src/magic.c#L8288)) items in shops and high-level zones (CRL / Shanat Citadel).
     - *Proposal*: Exponentially increase cost (e.g. 1M platinum) or gate behind long questlines. Currently unchanged.
  3. **`the bracer of the whirlwinds` (`VNUM 76032`)**:
     - *Mechanism*: Procs `AFF2_FLURRY` upon tapping/invoking ([`src/specs.object.c:11185`](file:///home/aiwithapex/projects/duris/src/specs.object.c#L11185)).
     - *Spawn Location*: Loaded in Tempest Court / Cosmic zone ([`areas/obj/cosmic.obj:411`](file:///home/aiwithapex/projects/duris/areas/obj/cosmic.obj#L411), [`areas/qst/cosmic.qst:61`](file:///home/aiwithapex/projects/duris/areas/qst/cosmic.qst#L61)).
     - *Proposal*: Move to Celestia and allow in Celestia Bartender Quests. Currently unchanged.

---

### Issue #3: Hellfire vs. Paladin
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/3`](https://github.com/xanadinn/DurisMUD/issues/3)
- **Status in Current Codebase**: **OUTSTANDING (Configurable via Properties)**
- **Technical Analysis**:
  - *Request*: *"Rebalance hellfire vs paladin heal proc"*.
  - *Mechanics*:
    - Spell Absorption: [`src/fight.c:4441-4450`](file:///home/aiwithapex/projects/duris/src/fight.c#L4441) absorbs spell damage and heals the `AFF4_HELLFIRE` victim based on `get_property("vamping.hellfire.absorb", 0.14)`.
    - Physical Damage Vamping: [`src/fight.c:5469`](file:///home/aiwithapex/projects/duris/src/fight.c#L5469) vamps damage dealt by `PHSDAM_HELLFIRE` based on `get_property("vamping.hellfire", 0.14)`.
  - No explicit balance patch was committed specifically changing Paladin holy weapon heal procs against Hellfire. The behavior is tunable dynamically at runtime via property overrides.

---

### Issue #4: Minotaur Balance
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/4`](https://github.com/xanadinn/DurisMUD/issues/4)
- **Status in Current Codebase**: **OUTSTANDING (Design Request)**
- **Technical Analysis**:
  - *Request*: *"Mino innates are fucked should be balanced against ogre or firbolg"*.
  - *Current State*:
    - Innates: `INNATE_ULTRAVISION`, `INNATE_CHARGE` (lvl 11), `INNATE_DOORBASH` (lvl 1), `INNATE_DAYVISION` (lvl 1) in [`src/innates.c:652-655`](file:///home/aiwithapex/projects/duris/src/innates.c#L652-L655).
    - Passive Rage: [`src/fight.c:7335`](file:///home/aiwithapex/projects/duris/src/fight.c#L7335) procs `TAG_MINOTAUR_RAGE`.
    - Horn Goring: [`src/fight.c:7549`](file:///home/aiwithapex/projects/duris/src/fight.c#L7549) procs headbutt/gore damage in melee combat.
    - Giant Wielding: Single-hands 2-handed weapons via `IS_GIANT(ch)` ([`src/actobj.c:94, 5448`](file:///home/aiwithapex/projects/duris/src/actobj.c#L94)).
  - No balancing patch vs Ogres or Firbolgs has been introduced.

---

### Issue #5: 3rd Racewar Side Re-Enabling
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/5`](https://github.com/xanadinn/DurisMUD/issues/5)
- **Status in Current Codebase**: **OUTSTANDING (Feature Request)**
- **Technical Analysis**:
  - *Request*: *"bring back undead or illithids (or both) depending on # of active players"*.
  - *Current State*:
    - The live roster operates as a standard 2-side war (Good vs Evil, with Neutral races choosing Good or Evil at character creation).
    - Undead forms exist as mid-game `descend` paths (Lich, Vampire, Deathknight, Wight, Revenant, Shadow Beast, Phantom, Shade per [`src/constant.c:1644-1654`](file:///home/aiwithapex/projects/duris/src/constant.c#L1644-L1654)).
    - Restricted player races (Illithid, Harpy, Sea Giant) are disabled from normal creation unless `CREATION_ALL_RACES=TRUE` is enabled in `.env`.
    - A dynamic, player-population-driven 3rd faction activation system does not exist in the codebase.

---

### Issue #6: Single-Hand Two-Handed Weapons
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/6`](https://github.com/xanadinn/DurisMUD/issues/6)
- **Status in Current Codebase**: **OUTSTANDING (Feature Request)**
- **Technical Analysis*:
  - *Request*: *"Re-balance single hand 2 handed weapon wielding. fumbles or skill training to enhance"*.
  - *Current State*:
    - In [`src/actobj.c:90-102`](file:///home/aiwithapex/projects/duris/src/actobj.c#L90-L102) (`wield_item_size`), any character matching `IS_GIANT(ch)` ([`src/utils.h:771`](file:///home/aiwithapex/projects/duris/src/utils.h#L771)) automatically treats `ITEM_TWOHANDS` and `WEAPON_2HANDSWORD` as size 1.
    - No fumble chance, hit penalties, or prerequisite weapon mastery skill requirements exist for giant-sized races wielding two-handers in one hand.

---

### Issue #7: Case-Sensitive Areas
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/7`](https://github.com/xanadinn/DurisMUD/issues/7)
- **Status in Current Codebase**: **PARTIALLY MITIGATED / ONGOING AUDIT**
- **Technical Analysis**:
  - *Current State*:
    - On Linux, area concatenation tools ([`areas/src/wld/make_wld.c`](file:///home/aiwithapex/projects/duris/areas/src/wld/make_wld.c), `make_mob.c`, `make_obj.c`, `make_zon.c`) use `fopen()` with filenames constructed directly from entries in [`areas/AREA`](file:///home/aiwithapex/projects/duris/areas/AREA).
    - All active entries currently in `areas/AREA` match their disk filenames exactly (`PortSkythic`, `Voluntown`, etc.).
    - However, several inactive/orphaned area files on disk use PascalCase (`Monsteri`, `Magetower`, `Mentiri`, `Nexus_1`, `IC3`). If added in lowercase to `areas/AREA`, `make_all` scripts will fail to locate them on case-sensitive filesystems.

---
