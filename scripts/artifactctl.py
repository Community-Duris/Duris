#!/usr/bin/env python3
"""Validate and publish the file-backed artifact control catalog.

The in-game ``artifact control`` command uses the same model.  This CLI is
intended for deployment checks and for operators who prepare a reviewable
catalog draft outside a running server.
"""

from __future__ import annotations

import argparse
import copy
import json
import os
import tempfile
from pathlib import Path
from typing import Any


DEFAULT_CATALOG = Path("lib/artifacts/catalog.json")
DEFAULT_DRAFT = Path("lib/artifacts/catalog.json.draft")
FNV_OFFSET = 1469598103934665603
FNV_PRIME = 1099511628211


class CatalogError(ValueError):
    pass


def load(path: Path) -> dict[str, Any]:
    try:
        with path.open(encoding="utf-8") as stream:
            value = json.load(stream)
    except (OSError, json.JSONDecodeError) as exc:
        raise CatalogError(f"cannot read {path}: {exc}") from exc
    validate(value, check_hash=True)
    return value


def _id(value: Any) -> bool:
    return (
        isinstance(value, str)
        and 0 < len(value) <= 63
        and value[0].islower()
        and all(ch.islower() or ch.isdigit() or ch in "_-." for ch in value)
    )


def validate(value: Any, check_hash: bool = True) -> None:
    if not isinstance(value, dict):
        raise CatalogError("catalog must be an object")
    if value.get("schemaVersion") != 1:
        raise CatalogError("schemaVersion must be 1")
    if not isinstance(value.get("revision"), int) or isinstance(value["revision"], bool) or value["revision"] < 1:
        raise CatalogError("revision must be a positive integer")
    definitions = value.get("definitions")
    if not isinstance(definitions, list):
        raise CatalogError("definitions must be an array")
    seen_vnums: set[int] = set()
    seen_ids: set[str] = set()
    for index, item in enumerate(definitions):
        prefix = f"definitions[{index}]"
        if not isinstance(item, dict):
            raise CatalogError(f"{prefix} must be an object")
        if not _id(item.get("id")) or not _id(item.get("uniquenessFamily")):
            raise CatalogError(f"{prefix} has an invalid id or uniquenessFamily")
        if item["id"] in seen_ids:
            raise CatalogError(f"duplicate artifact id {item['id']}")
        seen_ids.add(item["id"])
        vnum = item.get("vnum")
        if not isinstance(vnum, int) or isinstance(vnum, bool) or vnum <= 0:
            raise CatalogError(f"{prefix}.vnum must be positive")
        if vnum in seen_vnums:
            raise CatalogError(f"duplicate artifact vnum {vnum}")
        seen_vnums.add(vnum)
        if item.get("classification") not in {"major", "unique", "ioun"}:
            raise CatalogError(f"{prefix}.classification is invalid")
        if not 0 <= item.get("loadChance", -1) <= 100 or item.get("loadLimit", 0) < 1:
            raise CatalogError(f"{prefix} has invalid load controls")
        if not 0 <= item.get("initialLifetimeSeconds", -1) <= 31536000:
            raise CatalogError(f"{prefix}.initialLifetimeSeconds is invalid")
        if not 0 <= item.get("maxRemainingLifetimeSeconds", -1) <= 31536000:
            raise CatalogError(f"{prefix}.maxRemainingLifetimeSeconds is invalid")
        variants = item.get("variants")
        if not isinstance(variants, dict) or not variants:
            raise CatalogError(f"{prefix}.variants must be a non-empty object")
        for variant_id, variant in variants.items():
            if not _id(variant_id) or not isinstance(variant, dict) or not isinstance(variant.get("adapter"), str):
                raise CatalogError(f"{prefix}.variants.{variant_id} is invalid")
        holders = item.get("holderPolicy", {})
        if not isinstance(holders, dict):
            raise CatalogError(f"{prefix}.holderPolicy is invalid")
        for holder in ("player", "wildNpc", "controlledNpc"):
            selected = holders.get(holder, item.get("defaultVariant", "legacy"))
            if selected not in variants:
                raise CatalogError(f"{prefix}.holderPolicy.{holder} references an unknown variant")
        powers = item.get("powers", [])
        if not isinstance(powers, list):
            raise CatalogError(f"{prefix}.powers must be an array")
        power_ids: set[str] = set()
        for power in powers:
            if not isinstance(power, dict) or not _id(power.get("id")) or power["id"] in power_ids:
                raise CatalogError(f"{prefix} has duplicate or invalid power ids")
            power_ids.add(power["id"])
            numerator = power.get("chanceNumerator", 1)
            denominator = power.get("chanceDenominator", 1)
            if not isinstance(numerator, int) or not isinstance(denominator, int) or not 0 <= numerator <= denominator or denominator <= 0:
                raise CatalogError(f"{prefix}.powers.{power['id']} has invalid chance")
            if power.get("cooldownSeconds", 0) < 0 or power.get("cooldownSeconds", 0) > 604800:
                raise CatalogError(f"{prefix}.powers.{power['id']} has invalid cooldown")
            if power.get("windupPulses", 0) < 0 or power.get("windupPulses", 0) > 600:
                raise CatalogError(f"{prefix}.powers.{power['id']} has invalid windup")
            if power.get("manaCostMilliunits", 0) < 0 or not 0 <= power.get("powerLevel", 0) <= 60:
                raise CatalogError(f"{prefix}.powers.{power['id']} has invalid power limits")
    if check_hash and value.get("hash") not in (None, "") and value["hash"] != catalog_hash(value):
        raise CatalogError("catalog hash does not match its contents")


def _hash_input(value: dict[str, Any]) -> bytes:
    sep_field, sep_definition, sep_variant, sep_power = "\x1f", "\x1e", "\x1d", "\x1c"
    out = [str(value["schemaVersion"]), sep_field, str(value["revision"]), sep_definition]
    for item in sorted(value["definitions"], key=lambda entry: entry["vnum"]):
        holders = item.get("holderPolicy", {})
        fields = [
            item["id"], item["vnum"], item.get("displayName", item["id"]), item["classification"],
            item["uniquenessFamily"], item.get("defaultVariant", "legacy"),
            holders.get("player", item.get("defaultVariant", "legacy")),
            holders.get("wildNpc", item.get("defaultVariant", "legacy")),
            holders.get("controlledNpc", holders.get("player", item.get("defaultVariant", "legacy"))),
            item.get("legacyBinding", ""), item.get("source", ""), item.get("sourceLine", 0),
            item.get("initialLifetimeSeconds", 864000), item.get("maxRemainingLifetimeSeconds", 864000),
            item.get("loadChance", 100), item.get("loadLimit", 1), item.get("loadRoom", 0),
            item.get("loadMob", 0), item.get("loadSlot", -1), 1 if item.get("enabled", True) else 0,
        ]
        for field in fields[:-1]:
            out.extend((str(field), sep_field))
        out.extend((str(fields[-1]), sep_definition))
        for variant_id in sorted(item.get("variants", {})):
            out.extend((variant_id, sep_field, item["variants"][variant_id].get("adapter", ""), sep_definition))
        out.append(sep_variant)
        for power in sorted(item.get("powers", []), key=lambda entry: entry["id"]):
            out.extend(
                (power["id"], sep_field, 1 if power.get("enabled", True) else 0, sep_field,
                 power.get("chanceNumerator", 1), sep_field, power.get("chanceDenominator", 1), sep_field,
                 power.get("cooldownSeconds", 0), sep_field, power.get("windupPulses", 0), sep_field,
                 power.get("manaCostMilliunits", 0), sep_field, power.get("powerLevel", 0), sep_definition)
            )
        out.append(sep_power)
    return "".join(map(str, out)).encode("utf-8")


def catalog_hash(value: dict[str, Any]) -> str:
    result = FNV_OFFSET
    for byte in _hash_input(value):
        result ^= byte
        result = (result * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return f"{result:016x}"


def atomic_save(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent, delete=False) as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")
        temporary = Path(stream.name)
    os.replace(temporary, path)


def draft_path(args: argparse.Namespace) -> Path:
    return Path(args.draft) if args.draft else Path(str(args.catalog) + ".draft")


def find(value: dict[str, Any], vnum: int) -> dict[str, Any]:
    for item in value["definitions"]:
        if item["vnum"] == vnum:
            return item
    raise CatalogError(f"artifact vnum {vnum} was not found")


def holder_key(text: str) -> str:
    normalized = text.lower().replace("-", "")
    if normalized in {"player", "pc"}:
        return "player"
    if normalized in {"npc", "wildnpc"}:
        return "wildNpc"
    if normalized in {"controlled", "controllednpc"}:
        return "controlledNpc"
    raise CatalogError("holder must be player, npc, or controlled")


def command(args: argparse.Namespace) -> int:
    catalog_path = Path(args.catalog)
    if args.command == "validate":
        value = load(catalog_path)
        print(f"valid revision={value['revision']} hash={catalog_hash(value)} definitions={len(value['definitions'])}")
        return 0
    if args.command == "status":
        value = load(catalog_path)
        print(f"revision={value['revision']} hash={catalog_hash(value)} path={catalog_path}")
        return 0
    if args.command == "list":
        value = load(Path(args.draft) if args.draft else catalog_path)
        needle = (args.filter or "").lower()
        for item in sorted(value["definitions"], key=lambda entry: entry["vnum"]):
            if (needle and needle not in str(item["vnum"]).lower() and
                    needle not in item["id"].lower() and
                    needle not in item.get("displayName", "").lower() and
                    needle not in item.get("classification", "").lower()):
                continue
            holders = item.get("holderPolicy", {})
            print(f"{item['vnum']} {item['displayName']} [{item['classification']}] player={holders.get('player')} npc={holders.get('wildNpc')}")
        return 0
    if args.command == "inspect":
        value = load(Path(args.draft) if args.draft else catalog_path)
        item = find(value, args.vnum)
        print(json.dumps(item, indent=2, sort_keys=True))
        return 0
    if args.command in {"set-mode", "enable"}:
        draft = draft_path(args)
        value = load(catalog_path) if args.publish or not draft.exists() else load(draft)
        item = find(value, args.vnum)
        if args.command == "set-mode":
            selected = holder_key(args.holder)
            if args.variant not in item["variants"]:
                raise CatalogError(f"variant {args.variant} is not supported by {item['id']}")
            item.setdefault("holderPolicy", {})[selected] = args.variant
        else:
            item["enabled"] = bool(args.enabled)
        validate(value, check_hash=False)
        value["hash"] = ""
        target = catalog_path if args.publish else draft_path(args)
        if args.publish:
            value["revision"] += 1
            value["hash"] = catalog_hash(value)
        atomic_save(target, value)
        print(f"{'published' if args.publish else 'staged'} {target} revision={value['revision']} hash={value['hash'] or catalog_hash(value)}")
        return 0
    if args.command == "publish":
        source = draft_path(args)
        value = load(source)
        active = load(catalog_path)
        value["revision"] = active["revision"] + 1
        value["hash"] = catalog_hash(value)
        atomic_save(catalog_path, value)
        print(f"published {catalog_path} revision={value['revision']} hash={value['hash']}")
        return 0
    raise CatalogError(f"unknown command {args.command}")


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(description=__doc__)
    root.add_argument("--catalog", default=str(DEFAULT_CATALOG))
    root.add_argument("--draft", help="draft path; defaults to <catalog>.draft")
    sub = root.add_subparsers(dest="command", required=True)
    sub.add_parser("validate")
    sub.add_parser("status")
    listing = sub.add_parser("list")
    listing.add_argument("filter", nargs="?")
    inspect = sub.add_parser("inspect")
    inspect.add_argument("vnum", type=int)
    mode = sub.add_parser("set-mode")
    mode.add_argument("vnum", type=int)
    mode.add_argument("holder")
    mode.add_argument("variant")
    mode.add_argument("--publish", action="store_true")
    enable = sub.add_parser("enable")
    enable.add_argument("vnum", type=int)
    enable.add_argument("enabled", type=int, choices=(0, 1))
    enable.add_argument("--publish", action="store_true")
    sub.add_parser("publish")
    return root


def main() -> int:
    args = parser().parse_args()
    try:
        return command(args)
    except CatalogError as exc:
        print(f"artifactctl: {exc}", file=os.sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
