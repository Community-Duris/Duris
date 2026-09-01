#!/usr/bin/env python3
"""Render a plain Markdown Chaos-mode equipment reference from JSON evidence."""
from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


LEGACY_POLICY_VIOLATIONS = {
    "artifact": [424, 430, 906, 921, 922, 1230, 6826, 16262, 23805, 28973, 51401, 67262, 67274, 87546, 87612, 139004],
    "unique_keyword": [424, 23805, 28973, 67262, 87546],
    "ioun_slot": [906, 921, 922, 59318],
}

RACE_SLOT_RULES = [
    ("Fingers / earrings", "Thri-Kreen", "Runtime rejects both finger and earring slots."),
    ("Body", "Thri-Kreen", "Runtime rejects the main body slot."),
    ("Head", "Minotaur, Illithid, Pillithid", "Runtime rejects the head slot."),
    ("Legs", "Drider, Centaur, Harpy, Ogre, Firbolg", "Runtime rejects the main legs slot."),
    ("Feet", "Drider, Thri-Kreen, Harpy, Minotaur", "Runtime rejects the main feet slot."),
    ("Arms", "Ogre, Firbolg", "Runtime rejects the main arms slot."),
    ("Third/fourth weapons and extra limbs", "HAS_FOUR_HANDS()", "Only emitted through optional slot variants."),
    ("Horse body", "INNATE_HORSE_BODY", "Only emitted when the runtime innate is present."),
    ("Tail", "Centaur, Minotaur, Psionic Beast, Kobold, Tiefling", "Only emitted when the runtime tail check passes."),
    ("Nose", "Minotaur", "Only emitted for Minotaur."),
    ("Horns", "Minotaur, Harpy, Psionic Beast, Tiefling", "Only emitted for the runtime horn-bearing races."),
    ("Spider body", "INNATE_SPIDER_BODY", "Only emitted when the runtime innate is present."),
    ("Rear legs / rear feet", "None in current runtime", "has_eq_slot() currently rejects both slots; documented as unavailable."),
]


def clean(value: Any) -> str:
    text = "" if value is None else str(value)
    return " ".join(text.replace("\r", " ").replace("\n", " ").split())


def cell(value: Any) -> str:
    return clean(value).replace("|", "\\|")


def signed(value: Any) -> str:
    try:
        number = int(value)
    except (TypeError, ValueError):
        return str(value)
    return f"{number:+d}" if number else "0"


def fmt_effects(item: dict[str, Any]) -> str:
    summary = item.get("effect_summary", {})
    parts: list[str] = []
    affects = summary.get("affects", {})
    if affects:
        parts.append(", ".join(f"{name} {signed(value)}" for name, value in sorted(affects.items())))
    statuses = [clean(status) for status in summary.get("statuses", []) if clean(status) and clean(status) != "AFF_NONE"]
    if statuses:
        parts.append("status: " + ", ".join(statuses))
    return "; ".join(parts) or "no named numeric/status effect in snapshot"


def fmt_item(item: dict[str, Any]) -> str:
    return (
        f"{item.get('name', 'unknown')} (VNUM {item.get('vnum')}; "
        f"observed {item.get('observed_players', 0)} players; "
        f"power {item.get('power_score', 0)}; risk {item.get('risk_score', 0)})"
    )


def render_effect_evidence(analysis: dict[str, Any]) -> list[str]:
    lines = ["## Observed effect priorities", "", "These are aggregate worn-item observations from the level-50+ cohort; they are not per-player records.", "", "### Persistent/status effects", "", "| Effect | Item instances | Players |", "|---|---:|---:|"]
    statuses = analysis.get("effect_prevalence", {}).get("statuses", {})
    for name, row in sorted(statuses.items(), key=lambda pair: (-pair[1].get("players", 0), -pair[1].get("item_instances", 0), pair[0]))[:20]:
        lines.append(f"| {cell(name)} | {row.get('item_instances', 0)} | {row.get('players', 0)} |")
    lines += ["", "### Numeric affects", "", "| Affect | Item instances | Players | Signed modifier total |", "|---|---:|---:|---:|"]
    affects = analysis.get("effect_prevalence", {}).get("affects", {})
    for name, row in sorted(affects.items(), key=lambda pair: (-pair[1].get("players", 0), -pair[1].get("item_instances", 0), pair[0]))[:24]:
        lines.append(f"| `{cell(name)}` | {row.get('item_instances', 0)} | {row.get('players', 0)} | {row.get('signed_modifier_total', 0)} |")
    lines += ["", "Interpretation: the recurring player pattern is broad survivability and mobility (`Prot Fire`, `Sense Life`, `Farsee`, `Detect Magic`, `Major Mental`, `Prot Acid`, `Haste`, `Fly`) combined with hit points, max-stat effects, and negative save modifiers for spell/paralysis/fear saves. The generator uses those observations as selection evidence, not as a promise that every profile receives every effect.", ""]
    return lines


def render_fundamentals(catalog: dict[str, Any]) -> list[str]:
    lines = ["## Fundamental class items", "", "These are separate from the ordinary wearable matrix because the runtime treats them as class tools or special support. Standard fundamentals are allowed explicit quest/no-sell exceptions only where named below.", ""]
    for profile in ("standard", "enhanceable"):
        lines += [f"### {profile}", "", "| Role | VNUM | Item | Classes/use | Enhanceable | Reason |", "|---|---:|---|---|:---:|---|"]
        fundamentals = catalog.get("fundamentals", {}).get(profile, {})
        book = fundamentals.get("spellbook")
        if book:
            lines.append(f"| Spellbook | {book['vnum']} | {cell(book['name'])} | Eight spellbook classes in standard; strict alternative in enhanceable | {'yes' if book.get('enhanceable') else 'no'} | {cell(book.get('reason'))} |")
        for instrument in fundamentals.get("bard_instruments", []):
            lines.append(f"| Bard {cell(instrument.get('instrument_type'))} | {instrument['vnum']} | {cell(instrument['name'])} | Bard | {'yes' if instrument.get('enhanceable') else 'no'} | {cell(instrument.get('reason'))} |")
        totem = fundamentals.get("shaman_totem")
        if totem:
            lines.append(f"| Shaman three-sphere totem | {totem['vnum']} | {cell(totem['name'])} | Shaman | {'yes' if totem.get('enhanceable') else 'no'} | {cell(totem.get('reason'))} |")
        lines.append("")
    lines += ["Standard spellbook note: VNUM 7 is dynamically filled by the existing master-spellbook loader. Its active area prototype was made belt-attachable while retaining take/hold. The strict alternative uses a normal beltable spellbook and preserves the existing level-one spell population behavior.", ""]
    return lines


def render_consumables(catalog: dict[str, Any]) -> list[str]:
    lines = ["## Shared consumable support", "", "The same bounded support pool is placed inside the starter bag for every class. Choices come from high-level carried or nested-container adoption; counts are intentionally starter quantities rather than observed stockpiles.", "", "| Category | VNUM | Item | Starter count | Observed players | Median carried quantity | Reason |", "|---|---:|---|---:|---:|---:|---|"]
    for item in catalog.get("consumables", []):
        lines.append(f"| {cell(item.get('category'))} | {item['vnum']} | {cell(item['name'])} | {item.get('count', 0)} | {item.get('observed_players', 0)} | {item.get('median_quantity', 0)} | {cell(item.get('reason'))} |")
    lines += ["", "Charged staves and wands were kept outside the normal support policy. Placeholder VNUM 1252 is absent.", ""]
    return lines


def render_profile(catalog: dict[str, Any], profile: str, class_ids: dict[str, Any]) -> list[str]:
    lines = [f"## Full {profile} equipment profiles", "", "`power` and `risk` are analyzer heuristics used to suppress extreme outliers. They are not the game’s combat formula and should be retuned after live Chaos playtesting.", ""]
    matrix = catalog.get("profiles", {}).get(profile, {})
    for class_name, _class_id in sorted(class_ids.items(), key=lambda pair: int(pair[1])):
        row = matrix[class_name]
        lines += [f"### {class_name}", "", f"Observed high-level characters: {row.get('observed_high_level_characters', 0)} ({row.get('evidence_status', 'unknown')})", "", "| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |", "|---|---:|---|---:|---:|---:|---|"]
        for item in row.get("equipment", []):
            details = fmt_effects(item)
            reason = cell(item.get("reason"))
            if reason and reason not in details:
                details = details + "; " + reason
            lines.append(f"| `{cell(item.get('slot_name'))}` | {item['vnum']} | {cell(item['name'])} | {item.get('observed_players', 0)} | {item.get('power_score', 0)} | {item.get('risk_score', 0)} | {cell(details)} |")
        if row.get("support_items"):
            lines += ["", "Support items:", "", "| Role | VNUM | Item |", "|---|---:|---|"]
            for item in row["support_items"]:
                lines.append(f"| {cell(item.get('role'))} | {item['vnum']} | {cell(item['name'])} |")
        missing = [decision for decision in row.get("decisions", []) if decision.get("status") == "missing"]
        if missing:
            lines += ["", "Missing slots:"]
            lines.extend(f"- `{decision.get('slot')}`: {cell(decision.get('reason'))}" for decision in missing)
        lines.append("")
    return lines


def render_optionals(catalog: dict[str, Any]) -> list[str]:
    lines = ["## Race/body-slot variations", "", "Normal class profiles intentionally use the common equipment path. Optional rows are appended only after runtime slot and class/race checks; this avoids granting a universal profile a horse/tail/horn/spider item it cannot use.", "", "| Profile | Variation | Slot | VNUM | Item | Runtime condition | Races/body plans | Status |", "|---|---|---|---:|---|---|---|---|"]
    for profile in ("standard", "enhanceable"):
        for item in catalog.get("optional_race_slot_variations", {}).get(profile, []):
            lines.append(f"| {profile} | {cell(item.get('variation'))} | `{cell(item.get('slot_name'))}` | {item.get('vnum', '')} | {cell(item.get('name', '—'))} | `{cell(item.get('condition'))}` | {cell(', '.join(item.get('races', [])))} | {cell(item.get('status'))} |")
    lines += ["", "### Runtime slot restrictions that remain intentional", "", "| Slot family | Affected races/body plans | Consequence |", "|---|---|---|"]
    for slot, races, consequence in RACE_SLOT_RULES:
        lines.append(f"| {cell(slot)} | {cell(races)} | {cell(consequence)} |")
    lines += ["", "These are body-plan rules in `has_eq_slot()`/`wear()`, not item-level race restrictions. The loader also calls `can_char_use_item()` as a defense-in-depth check.", ""]
    return lines


def render_exclusions(analysis: dict[str, Any]) -> list[str]:
    counts = analysis.get("diagnostics", {}).get("exclusion_counts", {})
    lines = ["## Exclusions and balance guardrails", "", "The pre-change source audit found 127 resolved placeholder positions and 17 distinct selected VNUMs violating the requested artifact/unique/Ioun policy. The generated profiles remove those rows.", "", "### Pre-change selected-profile violations", ""]
    for category, values in LEGACY_POLICY_VIOLATIONS.items():
        lines.append(f"- {category}: {', '.join(str(value) for value in values)}")
    lines += ["", "### Catalog-wide exclusions observed during analysis", "", "| Exclusion | Catalog rows |", "|---|---:|"]
    for name, count in sorted(counts.items(), key=lambda pair: (-pair[1], pair[0])):
        lines.append(f"| `{cell(name)}` | {count} |")
    lines += ["", "### Selection policy", "", "- Normal equipment excludes artifact flags, Ioun wear, `unique` keywords, item-level race restrictions, item-level class restrictions, transient/no-rent/no-show/no-sell objects, and quest objects.", "- Explicit standard fundamentals are the only named exceptions: the master spellbook and the six legendary bard instruments. The standard shaman totem is not an artifact/unique/Ioun object.", "- The enhanceable profile requires the configured boot-time enhance predicate: VNUM range, takeability, non-artifact/non-transient economics, allowed bitvector masks, non-charged gear type, and configured pool exclusions.", "- Per-item risk is capped at 4.0 in the generated catalog. Extreme outliers such as boots of speed VNUM 36753 are excluded from the starter profile even though they are common enough to appear in the source cohort.", "- The selected rows are intentionally not copied from one character wholesale; they are per-slot aggregate winners with low-risk portable fallbacks for classes that have few or no direct observations.", ""]
    return lines


def render_common(catalog: dict[str, Any]) -> list[str]:
    lines = ["## Cross-class common items", "", "These are the items selected for at least 80% of generated class profiles in that profile.", "", "| Profile | Slot | VNUM | Item | Class profiles |", "|---|---|---:|---|---:|"]
    for profile in ("standard", "enhanceable"):
        for _, item in sorted(catalog.get("common_by_slot", {}).get(profile, {}).items(), key=lambda pair: int(pair[0])):
            lines.append(f"| {profile} | `{cell(item.get('slot_name'))}` | {item['vnum']} | {cell(item['name'])} | {item.get('class_count')}/{item.get('class_total')} |")
    lines.append("")
    return lines


def render_method(analysis: dict[str, Any], catalog: dict[str, Any]) -> list[str]:
    cohort = analysis.get("cohort", {})
    counts = analysis.get("database_counts", {})
    source = analysis.get("source", {})
    lines = [
        "# DurisMUD Chaos-Mode Starting Gear Reference",
        "",
        "Issue #69: CHAOS Gear Sets",
        "",
        "Status: generated from the July 2026 production snapshot and wired into the local `chaos-eq-system` branch. This document is a design/reference artifact; live balance still needs Chaos playtesting.",
        "",
        "## Decision summary",
        "",
        "- Grant one nested starter bag through the existing durable creation-grant path instead of queueing every item as a separate critical write.",
        "- Provide two operator-selectable profiles: `standard` (observed high-end gear plus explicit class fundamentals) and `enhanceable` (strict boot-enhance-index-compatible alternatives).",
        "- Begin the Chaos grant after rules acceptance and the durable character baseline, before `CON_RMOTD`/`CON_PLAYING`, so persistence overlaps the last creation screens.",
        "- Do not block commands for this pre-entry grant; announce `Your Chaos Equipment has been prepared!!` only after durable completion and entry.",
        "- The approval state `CON_ACCEPTWAIT` remains present but `approve_mode` is currently 0; if approval is re-enabled, schedule only from its approved transition rather than granting rejected characters.",
        "- Keep normal equipment portable at the item-data level and apply runtime `can_char_use_item()` and body-slot checks before putting an object in the bag.",
        "- Treat a missing/unusable required object as a fail-closed kit failure; only expected race/body-slot omissions are skipped.",
        "- Keep consumables inside the bag, with bounded starter quantities based on high-level carried/container usage rather than copying observed stockpiles.",
        "- Treat Bard instruments, the master spellbook, and the Shaman three-sphere totem as explicit fundamentals instead of accidentally filtering them as ordinary gear.",
        "",
        "## Evidence snapshot",
        "",
        f"- Cohort: active characters at level >= {cohort.get('threshold', 50)}.",
        f"- Characters: {cohort.get('characters', 0)}; level 51+: {cohort.get('level_51_or_higher', 0)}; level 56+: {cohort.get('level_56_or_higher', 0)}; observed maximum: {cohort.get('max_level', 0)}.",
        f"- Static object sources: {analysis.get('static_reconciliation', {}).get('area_files', 0)} active area files and {analysis.get('static_reconciliation', {}).get('area_records', 0)} parsed object records.",
        f"- Candidate templates after database/static reconciliation: {len(analysis.get('candidates', []))}; generated class profiles: {len(catalog.get('profiles', {}).get('standard', {}))} standard and {len(catalog.get('profiles', {}).get('enhanceable', {}))} enhanceable.",
        f"- Archive inventory: `player_data` 785 rows; `player_items` 50,239 rows; `player_item_affects` 34,008 rows; `player_item_extra_descr` 289,015 rows; `wiki_objects` 19,661 rows; `classes` 31; `races` 101; all item rows were reachable through `container_id -> player_items.id`, with maximum observed nesting depth 3.",
        "- The archive contains duplicate object-UID groups, so object UID is not used as the container-tree key; row `id` is the authoritative parent/child identity.",
        f"- Analytical cohort query rows: `player_data` {counts.get('characters', 'n/a')}; `player_items` {counts.get('items', 'n/a')}; saved item affects {counts.get('item_affects', 'n/a')}; nested object metadata available in archive.",
        f"- Source selection: `{source.get('area_root', 'areas/obj')}` constrained by `{source.get('area_list', 'areas/AREA')}`; item affects use saved instance rows when present and prototype affects otherwise.",
        "",
        "## Analysis design",
        "",
        "1. Read the archived SQL dump through an isolated local MariaDB instance; the archive itself remains unchanged.",
        "2. Use `player_items.id` as the container-tree identity and `container_id` as its parent reference. Persisted equipment slots are 1-based; the analyzer maps them to runtime slots by subtracting one.",
        "3. Reconcile SQL/wiki templates with active `.obj` area sources and runtime-normalize armor/wear flags, including loader-derived belt/back flags.",
        "4. Analyze worn effects, saved numeric affects, permanent statuses, class/race restrictions, nested carried consumables, Bard instrument type IDs, Shaman sphere masks, and enhance configuration.",
        "5. Rank per-slot candidates by class support, global adoption, bounded power, and risk; reject policy violations before catalog output.",
        "6. Validate the generated catalog again from active area sources before emitting the C header.",
        "",
        "The audited `can_char_use_item()`/`wear()` admission path has no alignment- or racewar-specific item gate; the selected item profiles therefore do not add one. Racewar and alignment still exist in other character/gameplay systems, and body-plan slot rules are documented separately below.",
        "",
    ]
    return lines


def render_verification() -> list[str]:
    return [
        "## Implementation and verification notes",
        "",
        "### Runtime changes",
        "",
        "- `src/account/nanny.c`: replaced the old placeholder-heavy Chaos tables and per-item grant chain with generated class/profile data, optional body-slot variants, bounded consumables, runtime slot checks, class/race checks, fail-closed required-item validation, one nested root-bag grant, and pre-entry scheduling after the accepted-rules baseline.",
        "- `src/item/item_movement_transaction.c` / `.h`: added an explicit direct-to-self pre-entry grant mode that does not block commands and announces completion after entry, including completions retained until `player_ready()`.",
        "- `src/account/chaos_eq_data.h`: generated standard/enhanceable arrays for all 30 classes, optional variations, and shared consumables.",
        "- `src/combat/chaos_config.c` / `.h`: added `CHAOS_EQ_PROFILE=standard|enhanceable`; invalid values fail closed to standard.",
        "- `areas/obj/limbo.obj`: added belt attachment to master spellbook VNUM 7 while preserving take/hold.",
        "- `tests/async/test_chaos_new_character_kit.py`: validates all class arrays, active VNUMs, policy exclusions, class/wear compatibility, fundamentals, optional arrays, and one-root submission.",
        "- `tests/async/test_chaos_preentry_grant.py`: validates pre-entry scheduling, non-blocking command semantics, PID validation exception boundaries, and post-entry announcement behavior.",
        "- `tests/async/test_flatfile_chaos_new_character_kit.py`: exercises real account/character creation, pre-entry scheduling, post-entry readiness messaging, bag contents, consumable visibility, save, and clean shutdown using the generated Warrior profile.",
        "",
        "### Reproducible commands",
        "",
        "```text",
        "python3 scripts/chaos_eq_analyze.py --repo-root . --docker-container chaos-eq-analysis-mariadb --database duris_prod --level-threshold 50 --area-root areas/obj --area-list areas/AREA --enhance-config lib/enhance.cfg --output-dir <analysis-dir>",
        "python3 scripts/chaos_eq_catalog.py --analysis <analysis-dir>/analysis.json --output-dir <catalog-dir> --header-out <catalog-dir>/chaos_eq_data.h --repo-root .",
        "python3 scripts/chaos_eq_validate.py --catalog <catalog-dir>/catalog.json --repo-root .",
        "python3 tests/async/test_chaos_new_character_kit.py",
        "python3 tests/async/test_chaos_preentry_grant.py",
        "python3 tests/async/test_master_spellbook.py",
        "python3 tests/async/test_chaos_env_toggle.py",
        "python3 tests/async/test_flatfile_chaos_new_character_kit.py",
        "make world",
        "make -C src -j2",
        "```",
        "",
        "The verified results for this implementation snapshot are:",
        "- Analyzer: passed; 2,733 candidate templates and 131-character cohort.",
        "- Catalog generation and fail-closed validator: passed; zero validation issues.",
        "- All-class Chaos contract, pre-entry grant contract, master-spellbook, environment/profile, readiness, hardcore, and persistence contracts: passed.",
        "- Corrected isolated flat-file Chaos journey: passed; the grant was scheduled before game entry, the success message was emitted after entry, generated bag contents and consumable visibility were verified, save completed, and shutdown was clean.",
        "- Chaos-focused regression matrix: passed; `make build && make test TEST_MATCH=chaos TEST_JOBS=1` reported 6 passed, 0 failed.",
        "",
        "## Known limitations and follow-up",
        "",
        "- The July archive predates some current persistence columns; analysis deliberately uses the compatible direct player/wiki tables rather than current player-load SQL unchanged.",
        "- SQL wiki metadata does not contain every raw runtime field (notably all bitvector provenance and some loader-derived fields), so active area prototypes and the strict runtime loader checks remain authoritative.",
        "- Some status names are display aliases from the current wiki/effect table; unresolved aliases are retained as names rather than guessed into a spell flag.",
        "- The generated lists are balanced against the available snapshot, not against a live Chaos population. Review actual PvP, caster burst, mobility, save stacking, and consumable depletion after deployment to a test environment.",
        "- Standard fundamentals intentionally bypass ordinary enhance/economy filters only where explicitly named. Do not add new exceptions by placing another item in a class array; update the analyzer policy and validator together.",
        "",
    ]


def render(analysis: dict[str, Any], catalog: dict[str, Any]) -> str:
    class_ids = catalog.get("source", {}).get("class_ids") or analysis.get("class_ids", {})
    lines: list[str] = []
    lines.extend(render_method(analysis, catalog))
    lines.extend(render_effect_evidence(analysis))
    lines.extend(render_fundamentals(catalog))
    lines.extend(render_consumables(catalog))
    lines.extend(render_common(catalog))
    lines.extend(render_optionals(catalog))
    lines.extend(render_exclusions(analysis))
    lines.extend(render_profile(catalog, "standard", class_ids))
    lines.extend(render_profile(catalog, "enhanceable", class_ids))
    lines.extend(render_verification())
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis", required=True)
    parser.add_argument("--catalog", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    analysis = json.loads(Path(args.analysis).read_text(encoding="utf-8"))
    catalog = json.loads(Path(args.catalog).read_text(encoding="utf-8"))
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(render(analysis, catalog), encoding="utf-8")
    print(json.dumps({"output": str(output.resolve()), "bytes": output.stat().st_size, "lines": len(output.read_text(encoding='utf-8').splitlines())}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
