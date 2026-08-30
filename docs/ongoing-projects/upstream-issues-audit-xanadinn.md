# Upstream Origin Issues Audit (`xanadinn/DurisMUD`)

A technical audit of all 8 issues filed in upstream repository [`https://github.com/xanadinn/DurisMUD/issues`](https://github.com/xanadinn/DurisMUD/issues), evaluated against the current state of the Duris codebase.

---

## Executive Summary Matrix

| # | Upstream Issue Title | Upstream State | Current Codebase Status | Category |
|---|---|:---:|:---:|---|
| **#1** | [DEFINITIONS (Read First) OPIs/balance/zone issues and suggested solutions](https://github.com/xanadinn/DurisMUD/issues/1) | `OPEN` | **OUTSTANDING** | Content / Item Balance Design Proposals |
| **#2** | [Kobold Calming](https://github.com/xanadinn/DurisMUD/issues/2) | `OPEN` | **RESOLVED / MITIGATED** | Racial Innate Rebalance |
| **#3** | [Hellfire vs. Paladin](https://github.com/xanadinn/DurisMUD/issues/3) | `OPEN` | **OUTSTANDING** | Combat & Spell Mechanics Balance |
| **#4** | [Minotaur Balance](https://github.com/xanadinn/DurisMUD/issues/4) | `OPEN` | **OUTSTANDING** | Racial Stat / Innate Balance |
| **#5** | [3rd Racewar Side Re-Enabling](https://github.com/xanadinn/DurisMUD/issues/5) | `OPEN` | **OUTSTANDING** | Game Architecture / Feature Request |
| **#6** | [single hand 2 hand weapons (race/class)](https://github.com/xanadinn/DurisMUD/issues/6) | `OPEN` | **OUTSTANDING** | Equipment / Combat Mechanics Request |
| **#7** | [Case senistive areas](https://github.com/xanadinn/DurisMUD/issues/7) | `OPEN` | **PARTIALLY MITIGATED** | Build Tooling & Asset Auditing |
| **#8** | [Fix: some things to hopefully fix dim spells](https://github.com/xanadinn/DurisMUD/issues/8) | `CLOSED` | **RESOLVED & VERIFIED** | Bug Fix (Merged Upstream) |

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

### Issue #2: Kobold Calming
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/2`](https://github.com/xanadinn/DurisMUD/issues/2)
- **Status in Current Codebase**: **RESOLVED / MITIGATED IN CODE**
- **Technical Analysis**:
  - *Request*: *"Give kobold calming a cooldown or other limiting factor"*.
  - *Resolution*: Rather than adding an arbitrary timer or cooldown check to `INNATE_CALMING`, the innate was **disabled entirely from the racial definition**:
    - [`src/innates.c:578`](file:///home/aiwithapex/projects/duris/src/innates.c#L578): `// ADD_RACIAL_INNATE(INNATE_CALMING, RACE_KOBOLD, 1);`
    - [`src/innates.c:526`](file:///home/aiwithapex/projects/duris/src/innates.c#L526): `// ADD_RACIAL_INNATE(INNATE_CALMING, RACE_HALFLING, 1);`
  - In the current codebase, `INNATE_CALMING` is strictly assigned as a class innate to Monks ([`src/innates.c:887`](file:///home/aiwithapex/projects/duris/src/innates.c#L887)).

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

### Issue #8: Dimension Door Fixes
- **Upstream URL**: [`https://github.com/xanadinn/DurisMUD/issues/8`](https://github.com/xanadinn/DurisMUD/issues/8)
- **Status in Current Codebase**: **RESOLVED & VERIFIED (Closed Upstream)**
- **Technical Analysis**:
  - *Resolution Details*:
    - **Bidirectional Range Check**: Implemented in [`src/magic.c:5052-5054`](file:///home/aiwithapex/projects/duris/src/magic.c#L5052-L5054) (`how_close(ch->in_room, victim->in_room, distance) < 0 && how_close(victim->in_room, ch->in_room, distance) < 0`).
    - **Float Modifier Precision**: Fixed integer truncation when evaluating `get_property("spell.dim.perlevel.modifier", 1.35)` in [`src/magic.c:5048`](file:///home/aiwithapex/projects/duris/src/magic.c#L5048) and [`src/smagic.c:4425`](file:///home/aiwithapex/projects/duris/src/smagic.c#L4425).
    - **Probe Normalization**: Normalized failure messages to generic `"&+cYou failed.\n"` across all invalid targets to prevent probing mob status or presence.

---

## Upstream Pull Request Audit: PR #7 (`Case senistive areas`)

- **Upstream PR URL**: [`https://github.com/xanadinn/DurisMUD/pull/7`](https://github.com/xanadinn/DurisMUD/pull/7)
- **Branch**: `Community-Duris/DurisMUD:caseSenistiveAreas` $\rightarrow$ `xanadinn/DurisMUD:master`
- **Relevance to Current Codebase**: **NOT RELEVANT / FULLY INCORPORATED (100% Merged)**

### Technical Evaluation & Evidence

1. **Commit Lineage Verification**:
   - PR #7 contains **80 commits** representing development history and area file normalization from `Community-Duris`.
   - A complete commit-by-commit audit against our local repository verified that **all 80 commits (80/80, 100%) already exist in our Git tree**.

2. **Area Filename Normalization**:
   - The primary file renames in PR #7 targeted uppercase area files that broke on case-sensitive Linux filesystems:
     - `areas/mob/forofcon.MOB` $\rightarrow$ `areas/mob/forofcon.mob`
     - `areas/obj/forofcon.OBJ` $\rightarrow$ `areas/obj/forofcon.obj`
     - `areas/qst/forofcon.QST` $\rightarrow$ `areas/qst/forofcon.qst`
     - `areas/zon/forofcon.ZON` $\rightarrow$ `areas/zon/forofcon.zon`
     - `areas/mob/LORNECRO.mob` $\rightarrow$ `areas/mob/lornecro.mob`
     - `areas/obj/LORNECRO.obj` $\rightarrow$ `areas/obj/lornecro.obj`
     - `areas/qst/LORNECRO.qst` $\rightarrow$ `areas/qst/lornecro.qst`
     - `areas/wld/LORNECRO.wld` $\rightarrow$ `areas/wld/lornecro.wld`
     - `areas/zon/LORNECRO.zon` $\rightarrow$ `areas/zon/lornecro.zon`
   - In our current repository, all of these area files are already normalized to lowercase in `areas/mob/`, `areas/obj/`, `areas/qst/`, `areas/wld/`, and `areas/zon/`.

3. **Conclusion**:
   - PR #7 is **obsolete/redundant for this codebase** because our fork already includes every commit and patch contained in PR #7. No action or merge is required.

---

## Upstream Branch Comparison: `Community-Duris:caseSenistiveAreas...xanadinn:master`

- **Comparison URL**: [`https://github.com/Community-Duris/DurisMUD/compare/caseSenistiveAreas...xanadinn%3ADurisMUD%3Amaster`](https://github.com/Community-Duris/DurisMUD/compare/caseSenistiveAreas...xanadinn%3ADurisMUD%3Amaster)
- **Relevance to Current Codebase**: **NOT RELEVANT / ALL FUNCTIONAL CHANGES ALREADY PRESENT**

### Summary of the 9 Divergent Commits on `xanadinn:master`

`xanadinn:master` contains 9 commits that diverged from `Community-Duris:caseSenistiveAreas`. Each commit was audited against our codebase:

| Commit | Author | Message | Status in Current Codebase | Technical Details |
|---|---|---|:---:|---|
| **`d30fba10`** | Josh Collins | *Lots of changing to get building for Ubuntu 24 (WSL2)* | **INCORPORATED / SUPERSEDED** | Replaced deprecated `types.h` and legacy type aliases (`byte` $\rightarrow$ `uint8`, `is_empty` $\rightarrow$ `is_zone_empty`). Our codebase compiles cleanly with modern C++20 / GCC (`g++`). |
| **`93837a40`** | Josh Collins | *random changes* | **REVERTED UPSTREAM** | Immediately reverted in commit `8d0409ad`. Net difference is zero. |
| **`872ac07a`** | Josh Collins | *will this actually work? everything needs to be lower case omg* | **SUPERSEDED** | Partial area file lowercase rename attempt on `xanadinn:master`. All area files and tools in our codebase are already cleanly normalized to lowercase. |
| **`8d0409ad`** | Josh Collins | *Revert "random changes"* | **N/A (Reversion)** | Reverted `93837a40`. |
| **`d40f9952`** | Josh Collins | *Updating ignore file* | **INCORPORATED** | Added `.vscode` to `.gitignore`. Our `.gitignore` is already updated and comprehensive. |
| **`c55774e5`** | Josh Collins | *More gitignore changes* | **INCORPORATED** | Added Visual Studio / Windows debug build patterns to `.gitignore`. |
| **`729845f5`** | sainth | *change drow guild zone to never reset* | **ALREADY PRESENT** | Modified Arachdrathos Guilds zone ([`areas/zon/aracguil.zon:3`](file:///home/aiwithapex/projects/duris/areas/zon/aracguil.zon#L3)) reset mode from 2 to 0 (`36777 0 0 350 360 2`). Verified identical in our codebase. |
| **`57962348`** | sainth | *fix from arih to compile on ubuntu 25.04* | **ALREADY PRESENT** | Added `(unsigned long)` cast in `ShipObjHash` pointer hashing in [`src/ships/ship_utils.c:45, 58, 74`](file:///home/aiwithapex/projects/duris/src/ships/ship_utils.c#L45) to prevent 64-bit pointer truncation warnings. Verified identical in our codebase. |
| **`600024e8`** | Josh Collins | *Adding rested bonus to god noob spellup* | **ALREADY PRESENT** | Added `spell_rest` to `newb_spellup` in [`src/actwiz.c:10694`](file:///home/aiwithapex/projects/duris/src/actwiz.c#L10694). Verified present in our codebase. |

### Conclusion

There are **no unmerged or missing changes** from `https://github.com/Community-Duris/DurisMUD/compare/caseSenistiveAreas...xanadinn%3ADurisMUD%3Amaster`. Every valid fix (zone reset flags, 64-bit compiler warning fixes, spellup additions, modern build compatibility) is already present and active in this repository.

