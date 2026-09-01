#!/usr/bin/env python3
"""Analyze DurisMUD high-level equipment for Chaos-mode starter kits.

The analyzer is deliberately read-only.  It queries an authorized development
clone through the mariadb client, reconciles the wiki catalog with the active
AREA object sources, and writes aggregate/template-level evidence only.
"""
from __future__ import annotations

import argparse
import ast
import csv
import itertools
import json
import math
import os
import re
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable


ITEM_TYPE_NAMES = {
    1: "light",
    2: "scroll",
    3: "wand",
    4: "staff",
    5: "weapon",
    6: "fireweapon",
    7: "missile",
    8: "treasure",
    9: "armor",
    10: "potion",
    11: "worn",
    12: "other",
    13: "trash",
    14: "wall",
    15: "container",
    16: "note",
    17: "drinkcon",
    18: "key",
    19: "food",
    20: "money",
    21: "pen",
    22: "boat",
    23: "book",
    24: "corpse",
    25: "teleport",
    26: "timer",
    27: "vehicle",
    28: "ship",
    29: "switch",
    30: "quiver",
    31: "pick",
    32: "instrument",
    33: "spellbook",
    34: "totem",
    35: "storage",
    36: "scabbard",
    37: "shield",
    38: "bandage",
    39: "spawner",
    40: "herb",
    41: "pipe",
}

CONSUMABLE_TYPES = {2, 3, 4, 10, 17, 19, 38, 40, 41}
GEAR_TYPES = {5, 9, 11, 30, 32, 33, 34, 37}
INSTRUMENT_VALUES = {
    184: "flute",
    185: "lyre",
    186: "mandolin",
    187: "harp",
    188: "drums",
    189: "horn",
}
TOTEM_SPHERE_MASK = 1 | 2 | 4 | 8 | 16 | 32

STATUS_WEIGHTS = {
    "Fly": 8.0,
    "Prot Fire": 6.0,
    "Prot Acid": 4.0,
    "Prot Gas": 4.0,
    "Prot Elec": 4.0,
    "Prot Cold": 4.0,
    "Fireshield": 5.0,
    "Iceshield": 5.0,
    "Stone Skin": 5.0,
    "Major Physical": 5.0,
    "Minor Globe": 4.0,
    "Major Mental": 4.0,
    "Haste": 4.0,
    "Regenerate": 4.0,
    "Free Action": 4.0,
    "Waterbreath": 2.0,
    "Breathwater": 2.0,
    "Infravision": 1.0,
    "Ultravision": 1.0,
    "Farsee": 1.0,
    "Sense Life": 1.0,
    "Aware": 1.0,
}

SAVE_APPLY_NAMES = {
    "APPLY_SAVING_PARA": "svpara",
    "APPLY_SAVING_FEAR": "svfear",
    "APPLY_SAVING_SPELL": "svspell",
}
STAT_APPLY_NAMES = {
    "APPLY_STR": "str",
    "APPLY_DEX": "dex",
    "APPLY_AGI": "agi",
    "APPLY_CON": "con",
    "APPLY_POW": "pow",
    "APPLY_INT": "int",
    "APPLY_WIS": "wis",
    "APPLY_CHA": "cha",
    "APPLY_STR_MAX": "str_max",
    "APPLY_DEX_MAX": "dex_max",
    "APPLY_AGI_MAX": "agi_max",
    "APPLY_CON_MAX": "con_max",
    "APPLY_POW_MAX": "pow_max",
    "APPLY_INT_MAX": "int_max",
    "APPLY_WIS_MAX": "wis_max",
    "APPLY_CHA_MAX": "cha_max",
    "APPLY_HIT": "hit",
    "APPLY_HITROLL": "hitroll",
    "APPLY_DAMROLL": "damroll",
    "APPLY_AC": "ac",
}


def clean_text(value: str | None) -> str:
    if not value:
        return ""
    value = re.sub(r"&\+[A-Za-z0-9]", "", value)
    value = re.sub(r"&[A-Za-z0-9]", "", value)
    value = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", value)
    return " ".join(value.replace("\r", " ").replace("\n", " ").split())


def norm_words(value: str | None) -> str:
    return re.sub(r"[^a-z0-9]+", " ", clean_text(value).lower()).strip()


def to_int(value: Any, default: int = 0) -> int:
    if value is None or value == "" or value == "NULL":
        return default
    return int(value)


def json_values(value: Any) -> list[int]:
    if value is None:
        return [0] * 8
    try:
        parsed = json.loads(value) if isinstance(value, str) else value
        if not isinstance(parsed, list):
            return [0] * 8
        result = [to_int(item) for item in parsed[:8]]
        return result + [0] * (8 - len(result))
    except (TypeError, ValueError, json.JSONDecodeError):
        return [0] * 8


class SafeExpression:
    """Evaluate the integer subset used by Duris C preprocessor constants."""

    allowed_binops = (ast.Add, ast.Sub, ast.BitOr, ast.BitAnd, ast.LShift, ast.RShift, ast.Mult, ast.FloorDiv)
    allowed_unary = (ast.UAdd, ast.USub, ast.Invert)

    def __init__(self, values: dict[str, int]):
        self.values = values

    def eval(self, expression: str) -> int | None:
        expression = expression.strip()
        expression = re.sub(r"/\*.*?\*/", "", expression)
        expression = expression.split("//", 1)[0].strip()
        expression = expression.rstrip(";").strip()
        if not expression or "?" in expression or ":" in expression:
            return None
        try:
            node = ast.parse(expression, mode="eval").body
        except SyntaxError:
            return None
        try:
            return int(self.visit(node))
        except (KeyError, TypeError, ValueError, ZeroDivisionError):
            return None

    def visit(self, node: ast.AST) -> int:
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return int(node.value)
        if isinstance(node, ast.Name):
            return self.values[node.id]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, self.allowed_unary):
            value = self.visit(node.operand)
            if isinstance(node.op, ast.UAdd):
                return value
            if isinstance(node.op, ast.USub):
                return -value
            return ~value
        if isinstance(node, ast.BinOp) and isinstance(node.op, self.allowed_binops):
            left, right = self.visit(node.left), self.visit(node.right)
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            if isinstance(node.op, ast.BitOr):
                return left | right
            if isinstance(node.op, ast.BitAnd):
                return left & right
            if isinstance(node.op, ast.LShift):
                return left << right
            if isinstance(node.op, ast.RShift):
                return left >> right
            if isinstance(node.op, ast.Mult):
                return left * right
            return left // right
        raise TypeError(type(node).__name__)


def parse_defines(path: Path) -> dict[str, int]:
    values = {f"BIT_{n}": 1 << (n - 1) for n in range(1, 65)}
    raw: dict[str, str] = {}
    for line in path.read_text(errors="replace").splitlines():
        match = re.match(r"^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(.+)$", line)
        if match:
            raw[match.group(1)] = match.group(2).strip()
    evaluator = SafeExpression(values)
    for _ in range(8):
        changed = False
        for name, expression in raw.items():
            if name in values and name.startswith("BIT_"):
                continue
            result = evaluator.eval(expression)
            if result is not None and values.get(name) != result:
                values[name] = result
                changed = True
        if not changed:
            break
    return values


def flag_field(name: str) -> int | None:
    for prefix, field in (("AFF5_", 4), ("AFF4_", 3), ("AFF3_", 2), ("AFF2_", 1), ("AFF_", 0)):
        if name.startswith(prefix):
            return field
    return None


def parse_flag_maps(defines: dict[str, int]) -> tuple[list[dict[int, str]], dict[str, tuple[int, int]]]:
    names: list[dict[int, str]] = [dict() for _ in range(5)]
    lookup: dict[str, tuple[int, int]] = {}
    for name, value in defines.items():
        field = flag_field(name)
        if field is None or name.endswith("_NONE") or value <= 0:
            continue
        names[field].setdefault(value, name)
        lookup[name] = (field, value)
    return names, lookup


def display_flag_name(name: str) -> str:
    if name.startswith("AFF5_"):
        name = name[5:]
    elif name.startswith("AFF4_"):
        name = name[5:]
    elif name.startswith("AFF3_"):
        name = name[5:]
    elif name.startswith("AFF2_"):
        name = name[5:]
    elif name.startswith("AFF_"):
        name = name[4:]
    return " ".join(part.capitalize() for part in name.split("_"))


def decode_flags(masks: Iterable[int], names: list[dict[int, str]]) -> list[str]:
    result: list[str] = []
    for field, mask in enumerate(masks):
        for bit, name in sorted(names[field].items()):
            if mask & bit:
                result.append(display_flag_name(name))
    return result


def read_tilde(lines: list[str], index: int) -> tuple[str, int]:
    parts: list[str] = []
    while index < len(lines):
        line = lines[index]
        index += 1
        if "~" in line:
            before, _ = line.split("~", 1)
            parts.append(before)
            return "\n".join(parts), index
        parts.append(line)
    raise ValueError("unterminated tilde string")


def numeric_line(line: str) -> list[int] | None:
    tokens = line.strip().split()
    if not tokens or any(not re.fullmatch(r"-?\d+", token) for token in tokens):
        return None
    return [int(token) for token in tokens]


def collect_numbers(lines: list[str], index: int, count: int) -> tuple[list[int], int]:
    result: list[int] = []
    while len(result) < count:
        if index >= len(lines):
            raise ValueError(f"expected {count} integers, found {len(result)}")
        values = numeric_line(lines[index])
        if values is None:
            raise ValueError(f"expected numeric object data, got {lines[index]!r}")
        result.extend(values)
        index += 1
    return result[:count], index


@dataclass
class AreaObject:
    vnum: int
    source: str
    keywords: str
    short_description: str
    object_type: int
    material: int
    size: int
    space: int
    craftsmanship: int
    damres: int
    extra_flags: int
    wear_flags: int
    extra2_flags: int
    anti_flags: int
    anti2_flags: int
    values: list[int]
    weight: int
    cost: int
    condition: int
    bitvectors: list[int]
    affects: list[tuple[int, int]]
    duplicate_count: int = 1
    ambiguous: bool = False

    def runtime_normalized(self, constants: dict[str, int]) -> "AreaObject":
        values = list(self.values)
        object_type = self.object_type
        wear = self.wear_flags
        bitvectors = list(self.bitvectors)
        take = constants.get("ITEM_TAKE", 1)
        hold = constants.get("ITEM_HOLD", 1 << 14)
        attach_belt = constants.get("ITEM_ATTACH_BELT", 1 << 23)
        wear_back = constants.get("ITEM_WEAR_BACK", 1 << 22)
        armor = constants.get("ITEM_ARMOR", 9)
        worn = constants.get("ITEM_WORN", 11)
        container = constants.get("ITEM_CONTAINER", 15)
        drinkcon = constants.get("ITEM_DRINKCON", 17)
        quiver = constants.get("ITEM_QUIVER", 30)
        if object_type == armor and values[0] == 0:
            object_type = worn
        if wear & take and not wear & hold:
            wear |= hold
        if wear & hold and not wear & take:
            wear |= take
        words = set(norm_words(self.keywords).split())
        belt_words = {"bag", "sack", "tube", "case", "scabbard", "pouch"}
        if ((object_type == drinkcon and words.intersection({"canteen", "skin", "horn"})) or
                (object_type == container and words.intersection(belt_words) and values[0] < 25) or
                object_type == quiver):
            wear |= attach_belt
        if (object_type == container and "backpack" in words) or object_type == quiver:
            wear |= wear_back
        sleep = constants.get("AFF_SLEEP", 1 << 17)
        fear = constants.get("AFF_FEAR", 1 << 21)
        charm = constants.get("AFF_CHARM", 1 << 22)
        bitvectors[0] &= ~(sleep | fear | charm)
        return AreaObject(
            vnum=self.vnum,
            source=self.source,
            keywords=self.keywords,
            short_description=self.short_description,
            object_type=object_type,
            material=self.material,
            size=self.size,
            space=self.space,
            craftsmanship=self.craftsmanship,
            damres=self.damres,
            extra_flags=self.extra_flags,
            wear_flags=wear,
            extra2_flags=self.extra2_flags,
            anti_flags=self.anti_flags,
            anti2_flags=self.anti2_flags,
            values=values,
            weight=self.weight,
            cost=self.cost,
            condition=self.condition,
            bitvectors=bitvectors,
            affects=list(self.affects),
            duplicate_count=self.duplicate_count,
            ambiguous=self.ambiguous,
        )


def area_file_names(area_root: Path, area_list: Path | None) -> list[Path]:
    if area_list is None:
        return sorted(area_root.glob("*.obj"))
    result: list[Path] = []
    for line in area_list.read_text(errors="replace").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("*"):
            continue
        name = stripped.split()[0]
        if name.startswith("*"):
            continue
        candidate = area_root / f"{name}.obj"
        if candidate.exists():
            result.append(candidate)
    return result


def parse_area_file(path: Path) -> list[AreaObject]:
    lines = path.read_text(errors="replace").splitlines()
    result: list[AreaObject] = []
    index = 0
    while index < len(lines):
        line = lines[index].strip()
        if not re.fullmatch(r"#\d+", line):
            index += 1
            continue
        vnum = int(line[1:])
        index += 1
        # The AREA list terminates with a synthetic #9999999/$~ marker;
        # it is not an object record and has no tilde-string payload.
        if index < len(lines) and lines[index].strip() == "$~":
            break
        keywords, index = read_tilde(lines, index)
        short_description, index = read_tilde(lines, index)
        _, index = read_tilde(lines, index)
        _, index = read_tilde(lines, index)
        header, index = collect_numbers(lines, index, 11)
        values, index = collect_numbers(lines, index, 8)
        base, index = collect_numbers(lines, index, 3)
        bitvectors = [0, 0, 0, 0, 0]
        if index < len(lines):
            optional = numeric_line(lines[index])
            if optional is not None:
                index += 1
                for field, value in enumerate(optional[:4]):
                    bitvectors[field] = value
        affects: list[tuple[int, int]] = []
        marker = lines[index].strip() if index < len(lines) else "$~"
        while index < len(lines):
            marker = lines[index].strip()
            if marker == "B5":
                index += 1
                one, index = collect_numbers(lines, index, 1)
                bitvectors[4] = one[0]
                continue
            if marker == "E":
                index += 1
                _, index = read_tilde(lines, index)
                _, index = read_tilde(lines, index)
                continue
            if marker == "A":
                index += 1
                affect, index = collect_numbers(lines, index, 2)
                affects.append((affect[0], affect[1]))
                continue
            if marker == "T":
                index += 1
                _, index = collect_numbers(lines, index, 4)
                continue
            break
        result.append(
            AreaObject(
                vnum=vnum,
                source=path.name,
                keywords=keywords,
                short_description=short_description,
                object_type=header[0],
                material=header[1],
                size=header[2],
                space=header[3],
                craftsmanship=header[4],
                damres=header[5],
                extra_flags=header[6],
                wear_flags=header[7],
                extra2_flags=header[8],
                anti_flags=header[9],
                anti2_flags=header[10],
                values=values,
                weight=base[0],
                cost=base[1],
                condition=base[2],
                bitvectors=bitvectors,
                affects=affects,
            )
        )
    return result


def reconcile_area_objects(paths: list[Path], wiki: dict[int, dict[str, Any]], constants: dict[str, int]) -> tuple[dict[int, AreaObject], dict[str, Any]]:
    options: dict[int, list[AreaObject]] = defaultdict(list)
    parse_errors: list[str] = []
    record_count = 0
    for path in paths:
        try:
            records = parse_area_file(path)
            record_count += len(records)
            for record in records:
                options[record.vnum].append(record)
        except (OSError, ValueError) as error:
            parse_errors.append(f"{path.name}: {error}")

    selected: dict[int, AreaObject] = {}
    duplicate_details: list[dict[str, Any]] = []
    for vnum, candidates in options.items():
        row = wiki.get(vnum, {})
        wiki_type = to_int(row.get("type"), -1)
        wiki_name = norm_words(row.get("name"))

        def score(candidate: AreaObject) -> int:
            score_value = 0
            if candidate.object_type == wiki_type:
                score_value += 100
            if norm_words(candidate.short_description) == wiki_name:
                score_value += 80
            elif wiki_name and wiki_name in norm_words(candidate.short_description):
                score_value += 30
            if candidate.extra_flags == to_int(row.get("extra_flags"), -1):
                score_value += 15
            if candidate.anti_flags == to_int(row.get("anti_flags"), -1):
                score_value += 10
            if candidate.anti2_flags == to_int(row.get("anti_flags2"), -1):
                score_value += 10
            return score_value

        # VNUM 7 is defined by the runtime as the master spellbook.  The active
        # AREA source's limbo record is authoritative despite legacy wiki rows.
        if vnum == 7:
            master = [candidate for candidate in candidates
                      if candidate.object_type == constants.get("ITEM_SPELLBOOK", 33)
                      and "master spellbook" in norm_words(candidate.short_description)]
            if master:
                candidates = master

        ranked = sorted(candidates, key=score, reverse=True)
        winner = ranked[0]
        materially_different = any(
            (candidate.object_type, candidate.extra_flags, candidate.wear_flags,
             candidate.extra2_flags, candidate.anti_flags, candidate.anti2_flags,
             candidate.values, candidate.bitvectors, candidate.affects)
            != (winner.object_type, winner.extra_flags, winner.wear_flags,
                winner.extra2_flags, winner.anti_flags, winner.anti2_flags,
                winner.values, winner.bitvectors, winner.affects)
            for candidate in ranked[1:]
        )
        winner.duplicate_count = len(candidates)
        winner.ambiguous = len(candidates) > 1 and materially_different and score(ranked[0]) == score(ranked[1])
        selected[vnum] = winner.runtime_normalized(constants)
        if len(candidates) > 1:
            duplicate_details.append({
                "vnum": vnum,
                "count": len(candidates),
                "selected_source": winner.source,
                "ambiguous": winner.ambiguous,
                "sources": [candidate.source for candidate in candidates],
            })
    return selected, {
        "area_files": len(paths),
        "area_records": record_count,
        "selected_vnums": len(selected),
        "duplicate_vnums": duplicate_details,
        "parse_errors": parse_errors,
    }


class SqlReader:
    def __init__(self, args: argparse.Namespace):
        self.args = args

    def query(self, sql: str, columns: list[str]) -> list[dict[str, Any]]:
        if self.args.docker_container:
            command = [
                "docker", "exec", self.args.docker_container, "mariadb",
                "--protocol=socket", "--batch", "--raw", "--skip-column-names",
                "--binary-mode", "--default-character-set=utf8mb4", "-uroot",
                self.args.database, "--execute", sql,
            ]
        else:
            if not self.args.mysql_host:
                raise RuntimeError("use --docker-container or --mysql-host")
            command = [
                "mariadb", "--batch", "--raw", "--skip-column-names", "--binary-mode",
                "--default-character-set=utf8mb4", "--host", self.args.mysql_host,
                "--port", str(self.args.mysql_port), "--user", self.args.mysql_user,
                self.args.database, "--execute", sql,
            ]
        environment = os.environ.copy()
        if self.args.mysql_password_env and environment.get(self.args.mysql_password_env):
            environment["MYSQL_PWD"] = environment[self.args.mysql_password_env]
        completed = subprocess.run(command, capture_output=True, env=environment)
        if completed.returncode != 0:
            error = completed.stderr.decode("utf-8", "replace").strip()
            raise RuntimeError(f"mariadb query failed: {error[:1000]}")
        text = completed.stdout.decode("utf-8", "replace")
        rows: list[dict[str, Any]] = []
        for line in text.splitlines():
            if not line:
                continue
            values = line.split("\t")
            # MariaDB's raw batch output omits trailing NULL columns on some
            # client/server combinations; those columns are safe to restore as
            # NULL because all selected fields are nullable/defaulted.
            if len(values) < len(columns):
                values.extend(["NULL"] * (len(columns) - len(values)))
            if len(values) > len(columns):
                raise RuntimeError(
                    f"query returned {len(values)} columns, expected {len(columns)}: {sql[:120]}"
                )
            rows.append({column: (None if value == "NULL" else value) for column, value in zip(columns, values)})
        return rows


def build_sql_data(reader: SqlReader, threshold: int) -> dict[str, list[dict[str, Any]]]:
    data: dict[str, list[dict[str, Any]]] = {}
    data["characters"] = reader.query(
        f"SELECT pid,level,m_class,secondary_class,race,racewar,alignment FROM player_data "
        f"WHERE active=1 AND level>={threshold} ORDER BY pid",
        ["pid", "level", "m_class", "secondary_class", "race", "racewar", "alignment"],
    )
    data["items"] = reader.query(
        f"SELECT pi.id,pi.pid,pi.vnum,pi.equip_slot,pi.container_id,pi.quantity,pi.weight,pi.cost,pi.timer,"
        f"pi.extra_flags,pi.wear_flags,pi.item_type,pi.value0,pi.value1,pi.value2,pi.value3,pi.value4,pi.value5,pi.value6,pi.value7,"
        f"REPLACE(REPLACE(REPLACE(COALESCE(pi.name,''),CHAR(9),' '),CHAR(10),' '),CHAR(13),' '),"
        f"REPLACE(REPLACE(REPLACE(COALESCE(pi.short_descr,''),CHAR(9),' '),CHAR(10),' '),CHAR(13),' '),"
        f"pi.bitvector1,pi.bitvector2,pi.bitvector3,pi.bitvector4,pi.bitvector5,pi.obj_uid,pi.item_condition "
        f"FROM player_items pi JOIN player_data p ON p.pid=pi.pid "
        f"WHERE p.active=1 AND p.level>={threshold} ORDER BY pi.pid,pi.id",
        ["id", "pid", "vnum", "equip_slot", "container_id", "quantity", "weight", "cost", "timer",
         "extra_flags", "wear_flags", "item_type", "value0", "value1", "value2", "value3", "value4",
         "value5", "value6", "value7", "saved_name", "saved_short", "bitvector1", "bitvector2",
         "bitvector3", "bitvector4", "bitvector5", "obj_uid", "item_condition"],
    )
    data["item_affects"] = reader.query(
        f"SELECT ia.item_id,ia.location,ia.modifier FROM player_item_affects ia "
        f"JOIN player_items pi ON pi.id=ia.item_id JOIN player_data p ON p.pid=pi.pid "
        f"WHERE p.active=1 AND p.level>={threshold} ORDER BY ia.item_id,ia.id",
        ["item_id", "location", "modifier"],
    )
    data["wiki_objects"] = reader.query(
        "SELECT vnum,"
        "REPLACE(REPLACE(REPLACE(name,CHAR(9),' '),CHAR(10),' '),CHAR(13),' '),"
        "REPLACE(REPLACE(REPLACE(COALESCE(name_ansi,''),CHAR(9),' '),CHAR(10),' '),CHAR(13),' '),"
        "type,level,weight,extra_flags,wear_flags,anti_flags,anti_flags2,zone_number,obj_values "
        "FROM wiki_objects ORDER BY vnum",
        ["vnum", "name", "name_ansi", "type", "level", "weight", "extra_flags", "wear_flags", "anti_flags", "anti_flags2", "zone_number", "obj_values"],
    )
    data["wiki_affects"] = reader.query(
        "SELECT object_vnum,location,modifier FROM wiki_object_affects ORDER BY object_vnum,id",
        ["object_vnum", "location", "modifier"],
    )
    data["wiki_effects"] = reader.query(
        "SELECT object_vnum,effect_name FROM wiki_object_spell_effects ORDER BY object_vnum,id",
        ["object_vnum", "effect_name"],
    )
    data["wiki_slots"] = reader.query(
        "SELECT object_vnum,slot_id FROM wiki_object_slots ORDER BY object_vnum,id",
        ["object_vnum", "slot_id"],
    )
    data["wiki_classes"] = reader.query(
        "SELECT object_vnum,class_id,is_allowed FROM wiki_object_classes ORDER BY object_vnum,id",
        ["object_vnum", "class_id", "is_allowed"],
    )
    data["wiki_races"] = reader.query(
        "SELECT object_vnum,race_id,is_allowed FROM wiki_object_races ORDER BY object_vnum,id",
        ["object_vnum", "race_id", "is_allowed"],
    )
    data["classes"] = reader.query(
        "SELECT id,name,short_name,menu_char FROM classes ORDER BY id",
        ["id", "name", "short_name", "menu_char"],
    )
    data["races"] = reader.query(
        "SELECT id,name,short_name,abbrev,racewar,playable FROM races ORDER BY id",
        ["id", "name", "short_name", "abbrev", "racewar", "playable"],
    )
    return data


def parse_enhance_config(path: Path, defines: dict[str, int]) -> dict[str, Any]:
    config: dict[str, Any] = {
        "search_min": 1300,
        "search_max": 134000,
        "pool_excluded_zones": [],
        "pool_excluded_vnums": [],
        "allow_masks": [0, 0, 0, 0, 0],
    }
    section = ""
    _, flag_lookup = parse_flag_maps(defines)
    if not path.exists():
        return config
    for raw_line in path.read_text(errors="replace").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].strip()
            continue
        if "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        if section == "settings":
            try:
                parsed = int(value.split("#", 1)[0].strip())
            except ValueError:
                continue
            if key == "enhance.search.vnum.min":
                config["search_min"] = parsed
            elif key == "enhance.search.vnum.max":
                config["search_max"] = parsed
        elif section == "pool_exclude_zone" and key == "zone":
            try:
                config["pool_excluded_zones"].append(int(value))
            except ValueError:
                pass
        elif section == "pool_exclude_vnum" and key == "vnum":
            try:
                config["pool_excluded_vnums"].append(int(value))
            except ValueError:
                pass
        elif section.startswith("bitvector"):
            parts = [part.strip() for part in value.split(",")]
            if len(parts) < 2 or key not in flag_lookup:
                continue
            try:
                allowed = int(parts[1])
            except ValueError:
                continue
            if allowed:
                field, bit = flag_lookup[key]
                config["allow_masks"][field] |= bit
    return config


def class_bit(class_id: int) -> int:
    return 1 << (class_id - 1)


def race_bit(race_id: int) -> int:
    return 1 << (race_id - 1)


def object_class_allowed(obj: AreaObject | None, class_id: int, constants: dict[str, int]) -> bool:
    if obj is None:
        return False
    bit = class_bit(class_id)
    allowed_flag = constants.get("ITEM_ALLOWED_CLASSES", 1 << 10)
    if obj.extra_flags & allowed_flag:
        return bool(obj.anti_flags & bit)
    return not bool(obj.anti_flags & bit)


def object_race_portable(obj: AreaObject | None, playable_races: list[int], constants: dict[str, int]) -> tuple[bool, str | None]:
    if obj is None:
        return False, "missing_static_object"
    allowed_flag = constants.get("ITEM_ALLOWED_RACES", 1 << 9)
    listed = [race_id for race_id in playable_races if obj.anti2_flags & race_bit(race_id)]
    if obj.extra_flags & allowed_flag:
        if len(listed) != len(playable_races):
            return False, "allowed_race_list_is_not_portable"
    elif listed:
        return False, "anti_race_flag"
    return True, None


def enhance_rejection_reasons(obj: AreaObject | None, config: dict[str, Any], constants: dict[str, int]) -> list[str]:
    if obj is None:
        return ["missing_static_object"]
    reasons: list[str] = []
    if not (obj.vnum >= config["search_min"] and obj.vnum <= config["search_max"]):
        reasons.append("outside_boot_vnum_range")
    if not (obj.wear_flags & constants.get("ITEM_TAKE", 1)):
        reasons.append("not_takeable")
    for name in ("ITEM_ARTIFACT", "ITEM_NOSELL", "ITEM_NORENT", "ITEM_NOSHOW", "ITEM_TRANSIENT"):
        if obj.extra_flags & constants.get(name, 0):
            reasons.append(name.lower())
    if obj.extra2_flags & constants.get("ITEM2_QUESTITEM", 1 << 15):
        reasons.append("quest_item")
    if obj.object_type == constants.get("ITEM_STAFF", 4) and obj.values[3] > 0:
        reasons.append("charged_staff")
    if obj.object_type in {
        constants.get("ITEM_TREASURE", 8), constants.get("ITEM_POTION", 10),
        constants.get("ITEM_MONEY", 20), constants.get("ITEM_KEY", 18), constants.get("ITEM_WAND", 3),
    }:
        reasons.append(f"non_gear_type_{ITEM_TYPE_NAMES.get(obj.object_type, obj.object_type)}")
    for field, mask in enumerate(config["allow_masks"]):
        if obj.bitvectors[field] & ~mask:
            reasons.append(f"blocked_bitvector{field + 1}")
    if obj.vnum in config["pool_excluded_vnums"]:
        reasons.append("pool_excluded_vnum")
    if obj.vnum // 100 in config["pool_excluded_zones"]:
        reasons.append("pool_excluded_zone")
    return reasons


def item_exclusion_reasons(obj: AreaObject | None, constants: dict[str, int], playable_races: list[int]) -> list[str]:
    if obj is None:
        return ["missing_static_object"]
    reasons: list[str] = []
    if obj.vnum == 1252:
        reasons.append("placeholder_vnum_1252")
    if obj.extra_flags & constants.get("ITEM_ARTIFACT", 1 << 28):
        reasons.append("artifact")
    if obj.wear_flags & constants.get("ITEM_WEAR_IOUN", 1 << 28):
        reasons.append("ioun")
    words = set(norm_words(obj.keywords).split())
    if "unique" in words and "powerunique" not in words:
        reasons.append("unique_keyword")
    for name in ("ITEM_TRANSIENT", "ITEM_NORENT", "ITEM_NOSHOW", "ITEM_NOSELL"):
        if obj.extra_flags & constants.get(name, 0):
            reasons.append(name.lower())
    if obj.extra2_flags & constants.get("ITEM2_QUESTITEM", 1 << 15):
        reasons.append("quest_item")
    portable, reason = object_race_portable(obj, playable_races, constants)
    if not portable and reason:
        reasons.append(reason)
    return reasons


FUNDAMENTAL_ALLOWED_EXCLUSIONS = {
    "item_transient",
    "item_norent",
    "item_noshow",
    "item_nosell",
    "quest_item",
}


def fundamental_blocking_reasons(metric: dict[str, Any]) -> list[str]:
    return [
        reason for reason in metric.get("exclusion_reasons", [])
        if reason not in FUNDAMENTAL_ALLOWED_EXCLUSIONS
    ]


def apply_name(location: int, apply_names: dict[int, str]) -> str:
    return apply_names.get(location, f"apply_{location}")


def percentile(values: list[float], fraction: float) -> float:
    if not values:
        return 0.0
    values = sorted(values)
    if len(values) == 1:
        return values[0]
    position = (len(values) - 1) * fraction
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return values[lower]
    return values[lower] + (values[upper] - values[lower]) * (position - lower)


def make_effect_summary(
    affects: list[tuple[int, int]],
    effect_names: Iterable[str],
    bitvectors: list[int],
    flag_names: list[dict[int, str]],
    apply_names: dict[int, str],
) -> dict[str, Any]:
    totals: Counter[str] = Counter()
    for location, modifier in affects:
        totals[apply_name(location, apply_names)] += modifier
    statuses = {clean_text(effect) for effect in effect_names if effect}
    statuses.update(decode_flags(bitvectors, flag_names))
    status_clean = sorted(statuses)
    normal_power = sum(max(0, totals[name]) * 1.8 for name in totals if not name.endswith("_max") and name not in {"svpara", "svfear", "svspell"})
    max_power = sum(max(0, totals[name]) * 2.2 for name in totals if name.endswith("_max"))
    combat_power = sum(max(0, totals[name]) * weight for name, weight in {"hit": 0.10, "hitroll": 1.2, "damroll": 1.5, "ac": 0.08}.items())
    save_power = sum(max(0, -totals[name]) * 1.4 for name in ("svpara", "svfear", "svspell"))
    status_power = sum(STATUS_WEIGHTS.get(status, 0.8) for status in status_clean)
    density = len([name for name in status_clean if name not in {"AFF_NONE"}])
    raw_power = normal_power + max_power + combat_power + save_power + status_power
    risk = max(0.0, density - 5.0) * 1.5 + max(0.0, raw_power - 55.0) * 0.12
    tags: list[str] = []
    max_tags = [name for name, value in totals.items() if name.endswith("_max") and value > 0]
    stat_tags = [name for name, value in totals.items() if name in {"str", "dex", "agi", "con", "pow", "int", "wis", "cha"} and value > 0]
    save_tags = [name for name in ("svspell", "svpara", "svfear") if totals[name] < 0]
    if max_tags:
        tags.append("max-stat focus: " + ", ".join(sorted(max_tags)))
    if stat_tags:
        tags.append("stat focus: " + ", ".join(sorted(stat_tags)))
    if save_tags:
        tags.append("good saves: " + ", ".join(save_tags))
    highlighted = [status for status in status_clean if status in STATUS_WEIGHTS]
    if highlighted:
        tags.append("status: " + ", ".join(highlighted))
    if totals["hitroll"] > 0 or totals["damroll"] > 0:
        tags.append("offense: hit/damage")
    if totals["ac"] > 0:
        tags.append("physical protection")
    return {
        "affects": dict(sorted(totals.items())),
        "statuses": status_clean,
        "power_score": round(raw_power, 3),
        "risk_score": round(risk, 3),
        "effect_density": density,
        "reason_tags": tags,
    }


def wiki_matches_source(obj: AreaObject | None, wiki: dict[str, Any]) -> bool:
    return (
        obj is None
        or (
            to_int(wiki.get("type"), -1) == obj.object_type
            and norm_words(wiki.get("name")) == norm_words(obj.short_description)
        )
    )


def effective_item(
    row: dict[str, Any],
    obj: AreaObject | None,
    wiki: dict[str, Any],
    instance_affects: dict[int, list[tuple[int, int]]],
    wiki_affects: dict[int, list[tuple[int, int]]],
    wiki_effects: dict[int, list[str]],
    flag_names: list[dict[int, str]],
    apply_names: dict[int, str],
) -> dict[str, Any]:
    vnum = to_int(row["vnum"])
    prototype_values = list(obj.values if obj else json_values(wiki.get("obj_values")))
    values: list[int] = []
    for index in range(8):
        value = row.get(f"value{index}")
        values.append(prototype_values[index] if value is None else to_int(value))
    item_type = obj.object_type if obj else to_int(wiki.get("type"))
    if row.get("item_type") is not None:
        item_type = to_int(row["item_type"])
    if item_type == 0 and obj:
        item_type = obj.object_type
    bitvectors = list(obj.bitvectors if obj else [0] * 5)
    for index in range(5):
        value = row.get(f"bitvector{index + 1}")
        if value is not None:
            bitvectors[index] = to_int(value)
    source_matches_wiki = wiki_matches_source(obj, wiki)
    affects = instance_affects.get(to_int(row["id"]))
    if affects is None:
        affects = (wiki_affects.get(vnum, []) if source_matches_wiki else []) or (obj.affects if obj else [])
    effects = wiki_effects.get(vnum, []) if source_matches_wiki else []
    summary = make_effect_summary(affects, effects, bitvectors, flag_names, apply_names)
    return {
        "vnum": vnum,
        "item_type": item_type,
        "values": values,
        "bitvectors": bitvectors,
        "affect_summary": summary,
        "saved_name": clean_text(row.get("saved_name")),
        "saved_short": clean_text(row.get("saved_short")),
    }


def class_ids_for_character(row: dict[str, Any], class_rows: list[dict[str, Any]], defines: dict[str, int]) -> list[int]:
    mask = to_int(row["m_class"])
    result = [to_int(item["id"]) for item in class_rows if mask & class_bit(to_int(item["id"]))]
    return result or [0]


def root_item_id(item_id: int, items_by_id: dict[int, dict[str, Any]]) -> int | None:
    seen: set[int] = set()
    current = item_id
    while True:
        if current in seen:
            return None
        seen.add(current)
        row = items_by_id.get(current)
        if row is None:
            return None
        parent = to_int(row.get("container_id"), 0)
        if parent <= 0:
            return current
        current = parent


def summarize_consumable(metrics: dict[int, dict[str, Any]], objects: dict[int, AreaObject], constants: dict[str, int], profile: str) -> list[dict[str, Any]]:
    type_buckets: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for metric in metrics.values():
        type_id = metric["item_type"]
        if type_id not in CONSUMABLE_TYPES:
            continue
        if metric["exclusion_reasons"]:
            continue
        if profile == "enhanceable" and not metric["enhanceable"]:
            continue
        category = ITEM_TYPE_NAMES.get(type_id, str(type_id))
        type_buckets[category].append(metric)
    selected: list[dict[str, Any]] = []
    for category, candidates in sorted(type_buckets.items()):
        candidates.sort(key=lambda item: (item["observed_players"], item["quantity_total"], item["vnum"]), reverse=True)
        for metric in candidates[:5]:
            selected.append({
                "category": category,
                "vnum": metric["vnum"],
                "name": metric["name"],
                "observed_players": metric["observed_players"],
                "observed_share": metric["observed_share"],
                "median_quantity": metric["median_quantity"],
                "upper_quartile_quantity": metric["upper_quartile_quantity"],
                "reason": f"high-level carried/contained {category} adoption",
            })
    return selected


def choose_fundamentals(
    all_objects: dict[int, AreaObject],
    metrics_by_vnum: dict[int, dict[str, Any]],
    constants: dict[str, int],
    class_rows: list[dict[str, Any]],
    playable_races: list[int],
) -> list[dict[str, Any]]:
    output: list[dict[str, Any]] = []
    bard_id = next((to_int(row["id"]) for row in class_rows if clean_text(row["name"]).lower() == "bard"), 16)
    shaman_id = next((to_int(row["id"]) for row in class_rows if clean_text(row["name"]).lower() == "shaman"), 9)
    attach_belt = constants.get("ITEM_ATTACH_BELT", 1 << 23)
    spellbook = all_objects.get(7)
    output.append({
        "role": "spellbook_classes",
        "vnum": 7,
        "name": clean_text(spellbook.short_description) if spellbook else "master spellbook",
        "type": ITEM_TYPE_NAMES.get(spellbook.object_type, "unknown") if spellbook else "unknown",
        "classes": ["Sorcerer", "Conjurer", "Necromancer", "Illusionist", "Bard", "Summoner", "Reaver", "Theurgist"],
        "beltable": bool(spellbook and spellbook.wear_flags & attach_belt),
        "standard_eligible": bool(
            spellbook
            and not [
                reason for reason in item_exclusion_reasons(spellbook, constants, playable_races)
                if reason not in FUNDAMENTAL_ALLOWED_EXCLUSIONS
            ]
        ),
        "enhanceable": bool(metrics_by_vnum.get(7, {}).get("enhanceable", False)),
        "reason": "runtime master spellbook VNUM 7; dynamic spell loading is handled by the object loader",
        "blocking_reasons": [] if spellbook and spellbook.wear_flags & attach_belt else ["master_spellbook_not_beltable"],
    })

    for instrument_value, instrument_name in sorted(INSTRUMENT_VALUES.items()):
        candidates: list[dict[str, Any]] = []
        for vnum, obj in all_objects.items():
            if obj.object_type != constants.get("ITEM_INSTRUMENT", 32) or obj.values[0] != instrument_value:
                continue
            metric = metrics_by_vnum.get(vnum)
            if not metric or fundamental_blocking_reasons(metric) or not object_class_allowed(obj, bard_id, constants):
                continue
            candidates.append(metric)
        candidates.sort(key=lambda item: ("legendary" in item["name"].lower(), item["observed_players"], item["power_score"]), reverse=True)
        chosen = candidates[0] if candidates else None
        output.append({
            "role": "bard_instrument",
            "instrument_type": instrument_name,
            "vnum": chosen["vnum"] if chosen else None,
            "name": chosen["name"] if chosen else None,
            "standard_eligible": bool(chosen),
            "enhanceable": bool(chosen and chosen["enhanceable"]),
            "observed_players": chosen["observed_players"] if chosen else 0,
            "reason": "one validated instrument of each bard instrument type; legendary series preferred for standard profile",
            "alternative_vnum": next((item["vnum"] for item in candidates if item["enhanceable"]), None),
        })

    totem_candidates: list[dict[str, Any]] = []
    for vnum, obj in all_objects.items():
        if obj.object_type != constants.get("ITEM_TOTEM", 34) or not (obj.values[0] & TOTEM_SPHERE_MASK) == TOTEM_SPHERE_MASK:
            continue
        metric = metrics_by_vnum.get(vnum)
        if metric and not fundamental_blocking_reasons(metric) and object_class_allowed(obj, shaman_id, constants):
            totem_candidates.append(metric)
    totem_candidates.sort(key=lambda item: (item["observed_players"], item["power_score"]), reverse=True)
    totem = totem_candidates[0] if totem_candidates else None
    output.append({
        "role": "shaman_three_sphere_totem",
        "vnum": totem["vnum"] if totem else None,
        "name": totem["name"] if totem else None,
        "standard_eligible": bool(totem),
        "enhanceable": bool(totem and totem["enhanceable"]),
        "observed_players": totem["observed_players"] if totem else 0,
        "reason": "value0 contains lesser/greater animal, elemental, and spirit sphere bits (mask 63)",
        "alternative_vnum": next((item["vnum"] for item in totem_candidates if item["enhanceable"]), None),
    })
    return output


def build_recommendations(
    metrics: dict[int, dict[str, Any]],
    character_classes: dict[int, list[int]],
    class_rows: list[dict[str, Any]],
    profile: str,
) -> dict[str, list[dict[str, Any]]]:
    class_names = {to_int(row["id"]): clean_text(row["name"]) for row in class_rows}
    class_pids: dict[int, set[int]] = defaultdict(set)
    for pid, class_ids in character_classes.items():
        for class_id in class_ids:
            if class_id:
                class_pids[class_id].add(pid)
    results: dict[str, list[dict[str, Any]]] = {}
    for class_id in sorted(class_names):
        candidates_by_slot: dict[int, list[tuple[float, dict[str, Any], int]]] = defaultdict(list)
        total_class = max(1, len(class_pids[class_id]))
        for metric in metrics.values():
            if metric["exclusion_reasons"]:
                continue
            if profile == "enhanceable" and not metric["enhanceable"]:
                continue
            if not metric["race_portable"]:
                continue
            for slot, pids in metric.get("_slot_players", {}).items():
                if not (1 <= int(slot) <= 42):
                    continue
                class_support = len(set(pids) & class_pids[class_id])
                global_support = len(pids)
                if class_support == 0 and not class_pids[class_id]:
                    class_support = global_support
                if class_support == 0:
                    # A class with no direct observation may use a portable,
                    # class-legal global candidate as an explicit fallback.
                    if not metric["class_eligible"].get(str(class_id), False):
                        continue
                    support = global_support / max(1, len(character_classes))
                else:
                    support = class_support / total_class
                score = (support * 100.0 + math.log1p(global_support) * 2.0
                         + min(metric["power_score"], 50.0) * 0.55
                         - metric["risk_score"] * 0.85)
                candidates_by_slot[slot].append((score, metric, class_support))
        chosen: list[dict[str, Any]] = []
        for slot, candidates in sorted(candidates_by_slot.items()):
            candidates.sort(key=lambda value: (value[0], value[1]["observed_players"], value[1]["vnum"]), reverse=True)
            score, metric, class_support = candidates[0]
            chosen.append({
                "slot": slot,
                "vnum": metric["vnum"],
                "name": metric["name"],
                "observed_players": metric["observed_players"],
                "class_support": class_support,
                "score": round(score, 3),
                "power_score": metric["power_score"],
                "risk_score": metric["risk_score"],
                "effect_summary": metric["effect_summary"],
                "reason": metric["reason"],
            })
        results[class_names[class_id]] = chosen
    return results


def write_csv(path: Path, rows: list[dict[str, Any]], fields: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def analyze(args: argparse.Namespace) -> dict[str, Any]:
    root = Path(args.repo_root).resolve()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    defines = parse_defines(root / "src/core/defines.h")
    flag_names, flag_lookup = parse_flag_maps(defines)
    wanted_apply_names = set(STAT_APPLY_NAMES) | set(SAVE_APPLY_NAMES)
    apply_display: dict[int, str] = {}
    for name, value in defines.items():
        if not name.startswith("APPLY_") or value <= 0:
            continue
        display = STAT_APPLY_NAMES.get(name, SAVE_APPLY_NAMES.get(name, name.lower()))
        apply_display.setdefault(value, display)
        if name in wanted_apply_names:
            apply_display[value] = display
    sql = build_sql_data(SqlReader(args), args.level_threshold)
    wiki = {to_int(row["vnum"]): row for row in sql["wiki_objects"]}
    area_root = (root / args.area_root).resolve()
    area_list = (root / args.area_list).resolve() if args.area_list else None
    area_paths = area_file_names(area_root, area_list)
    area_objects, area_diagnostics = reconcile_area_objects(area_paths, wiki, defines)
    enhance_config = parse_enhance_config((root / args.enhance_config).resolve(), defines)
    playable_races = [to_int(row["id"]) for row in sql["races"] if to_int(row["id"]) > 0 and to_int(row["playable"]) == 1]
    class_rows = [row for row in sql["classes"] if to_int(row["id"]) > 0]
    character_rows = sql["characters"]
    characters = {to_int(row["pid"]): row for row in character_rows}
    character_classes = {pid: class_ids_for_character(row, class_rows, defines) for pid, row in characters.items()}
    class_names = {to_int(row["id"]): clean_text(row["name"]) for row in class_rows}
    wiki_affects: dict[int, list[tuple[int, int]]] = defaultdict(list)
    for row in sql["wiki_affects"]:
        wiki_affects[to_int(row["object_vnum"])].append((to_int(row["location"]), to_int(row["modifier"])))
    wiki_effects: dict[int, list[str]] = defaultdict(list)
    for row in sql["wiki_effects"]:
        wiki_effects[to_int(row["object_vnum"])].append(clean_text(row["effect_name"]))
    instance_affects: dict[int, list[tuple[int, int]]] = defaultdict(list)
    for row in sql["item_affects"]:
        instance_affects[to_int(row["item_id"])].append((to_int(row["location"]), to_int(row["modifier"])))

    item_rows_by_pid: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for row in sql["items"]:
        item_rows_by_pid[to_int(row["pid"])].append(row)
    diagnostics: dict[str, Any] = {
        "missing_wiki_vnums": [],
        "missing_area_vnums": [],
        "orphan_container_items": 0,
        "container_cycles": 0,
        "static_ambiguities": [item for item in area_diagnostics["duplicate_vnums"] if item["ambiguous"]],
        "area": area_diagnostics,
    }
    usage: dict[int, dict[str, Any]] = {}
    carried_observations: list[dict[str, Any]] = []
    worn_observations: list[dict[str, Any]] = []
    for pid, rows in item_rows_by_pid.items():
        by_id = {to_int(row["id"]): row for row in rows}
        for row in rows:
            vnum = to_int(row["vnum"])
            obj = area_objects.get(vnum)
            if obj is None:
                diagnostics["missing_area_vnums"].append(vnum)
            if vnum not in wiki:
                diagnostics["missing_wiki_vnums"].append(vnum)
            effective = effective_item(
                row, obj, wiki.get(vnum, {}), instance_affects, wiki_affects,
                wiki_effects, flag_names, apply_display,
            )
            observation = {
                "pid": pid,
                "item_id": to_int(row["id"]),
                "vnum": vnum,
                # player_items persists equipment slots as 1..MAX_WEAR;
                # runtime WEAR_* constants are the persisted value minus one.
                "slot": max(0, to_int(row["equip_slot"]) - 1),
                "persisted_equip_slot": to_int(row["equip_slot"]),
                "container_id": to_int(row["container_id"], 0),
                "quantity": max(1, to_int(row["quantity"], 1)),
                "item_type": effective["item_type"],
                "values": effective["values"],
                "affect_summary": effective["affect_summary"],
            }
            if observation["persisted_equip_slot"] > 0:
                worn_observations.append({**observation, "observation_kind": "worn"})
            else:
                root_id = root_item_id(observation["item_id"], by_id)
                if root_id is None:
                    if observation["container_id"]:
                        diagnostics["container_cycles"] += 1
                    continue
                if observation["container_id"] and observation["container_id"] not in by_id:
                    diagnostics["orphan_container_items"] += 1
                    continue
                carried_observations.append({**observation, "root_item_id": root_id, "observation_kind": "carried"})

    cohort_size = len(characters)
    quantity_by_vnum_pid: dict[int, dict[int, int]] = defaultdict(lambda: defaultdict(int))
    profile_counts: dict[int, Counter[str]] = defaultdict(Counter)
    for observation in worn_observations + carried_observations:
        vnum = observation["vnum"]
        obj = area_objects.get(vnum)
        metric = usage.setdefault(vnum, {
            "vnum": vnum,
            "name": clean_text(obj.short_description) if obj else clean_text(wiki.get(vnum, {}).get("name")) or f"VNUM {vnum}",
            "item_type": observation["item_type"],
            "slots": Counter(),
            "slot_players": defaultdict(set),
            "players": set(),
            "carried_players": set(),
            "worn_players": set(),
            "quantities": defaultdict(list),
            "effect_profiles": Counter(),
            "effect_summary": observation["affect_summary"],
        })
        metric["players"].add(observation["pid"])
        metric["quantities"][observation["pid"]].append(observation["quantity"])
        profile_key = json.dumps(observation["affect_summary"], sort_keys=True)
        metric["effect_profiles"][profile_key] += 1
        if observation["observation_kind"] == "carried":
            metric["carried_players"].add(observation["pid"])
            quantity_by_vnum_pid[vnum][observation["pid"]] += observation["quantity"]
        if observation["observation_kind"] == "worn":
            metric["worn_players"].add(observation["pid"])
            slot = observation["slot"]
            metric["slots"][slot] += 1
            metric["slot_players"][slot].add(observation["pid"])

    # Static-only fundamentals must also be visible to the catalog.  Add every
    # instrument/totem/spellbook in the loaded object universe, but keep the
    # full metric list limited to observed cohort gear elsewhere.
    for vnum, obj in area_objects.items():
        if obj.object_type not in {32, 33, 34}:
            continue
        usage.setdefault(vnum, {
            "vnum": vnum,
            "name": clean_text(obj.short_description),
            "item_type": obj.object_type,
            "slots": Counter(),
            "slot_players": defaultdict(set),
            "players": set(),
            "carried_players": set(),
            "worn_players": set(),
            "quantities": defaultdict(list),
            "effect_profiles": Counter(),
            "effect_summary": make_effect_summary(
                (wiki_affects.get(vnum, []) if wiki_matches_source(obj, wiki.get(vnum, {})) else []) or obj.affects,
                wiki_effects.get(vnum, []) if wiki_matches_source(obj, wiki.get(vnum, {})) else [],
                obj.bitvectors,
                flag_names, apply_display,
            ),
        })

    metrics: dict[int, dict[str, Any]] = {}
    for vnum, metric in usage.items():
        obj = area_objects.get(vnum)
        metric["observed_players"] = len(metric["players"])
        metric["observed_share"] = round(metric["observed_players"] / cohort_size, 4) if cohort_size else 0.0
        metric["worn_players_count"] = len(metric["worn_players"])
        metric["carried_players_count"] = len(metric["carried_players"])
        metric["_slot_players"] = {
            str(slot): set(pids) for slot, pids in metric["slot_players"].items()
        }
        metric["slot_players"] = {
            str(slot): len(pids) for slot, pids in metric["_slot_players"].items()
        }
        metric["slots"] = {str(slot): count for slot, count in metric["slots"].items()}
        metric["quantity_total"] = sum(sum(values) for values in metric["quantities"].values())
        quantity_values = [sum(values) for values in metric["quantities"].values()]
        metric["median_quantity"] = round(percentile([float(value) for value in quantity_values], 0.5), 2) if quantity_values else 0
        metric["upper_quartile_quantity"] = round(percentile([float(value) for value in quantity_values], 0.75), 2) if quantity_values else 0
        metric["effect_summary"] = json.loads(metric["effect_profiles"].most_common(1)[0][0]) if metric["effect_profiles"] else metric["effect_summary"]
        metric["exclusion_reasons"] = item_exclusion_reasons(obj, defines, playable_races)
        metric["race_portable"], _ = object_race_portable(obj, playable_races, defines)
        metric["enhance_rejections"] = enhance_rejection_reasons(obj, enhance_config, defines)
        metric["enhanceable"] = not metric["enhance_rejections"]
        metric["class_eligible"] = {
            str(class_id): object_class_allowed(obj, class_id, defines)
            for class_id in class_names
        }
        metric["power_score"] = metric["effect_summary"]["power_score"]
        metric["risk_score"] = metric["effect_summary"]["risk_score"]
        metric["reason"] = "; ".join(metric["effect_summary"]["reason_tags"][:4]) or "observed high-level equipment usage"
        metric["type_name"] = ITEM_TYPE_NAMES.get(metric["item_type"], str(metric["item_type"]))
        metric["static"] = {
            "source": obj.source if obj else None,
            "material": obj.material if obj else None,
            "craftsmanship": obj.craftsmanship if obj else None,
            "values": obj.values if obj else json_values(wiki.get(vnum, {}).get("obj_values")),
            "extra_flags": obj.extra_flags if obj else to_int(wiki.get(vnum, {}).get("extra_flags")),
            "wear_flags": obj.wear_flags if obj else to_int(wiki.get(vnum, {}).get("wear_flags")),
            "extra2_flags": obj.extra2_flags if obj else None,
            "anti_flags": obj.anti_flags if obj else to_int(wiki.get(vnum, {}).get("anti_flags")),
            "anti2_flags": obj.anti2_flags if obj else to_int(wiki.get(vnum, {}).get("anti_flags2")),
            "bitvectors": obj.bitvectors if obj else None,
            "statuses": metric["effect_summary"]["statuses"],
        }
        metrics[vnum] = metric

    serializable_metrics: list[dict[str, Any]] = []
    exclusion_counts: Counter[str] = Counter()
    for metric in sorted(metrics.values(), key=lambda item: item["vnum"]):
        for reason in metric["exclusion_reasons"]:
            exclusion_counts[reason] += 1
        serializable_metrics.append({
            key: value
            for key, value in metric.items()
            if key not in {"players", "carried_players", "worn_players", "quantities", "effect_profiles", "_slot_players"}
        })

    worn_status_instances: Counter[str] = Counter()
    worn_status_players: dict[str, set[int]] = defaultdict(set)
    worn_affect_instances: Counter[str] = Counter()
    worn_affect_totals: Counter[str] = Counter()
    worn_affect_players: dict[str, set[int]] = defaultdict(set)
    for observation in worn_observations:
        summary = observation["affect_summary"]
        for status in summary["statuses"]:
            worn_status_instances[status] += 1
            worn_status_players[status].add(observation["pid"])
        for affect, modifier in summary["affects"].items():
            worn_affect_instances[affect] += 1
            worn_affect_totals[affect] += modifier
            worn_affect_players[affect].add(observation["pid"])

    worn_valid_by_pid: dict[int, set[int]] = defaultdict(set)
    for observation in worn_observations:
        metric = metrics.get(observation["vnum"])
        if metric and not item_exclusion_reasons(area_objects.get(observation["vnum"]), defines, playable_races):
            worn_valid_by_pid[observation["pid"]].add(observation["vnum"])
    pair_counts: Counter[tuple[int, int]] = Counter()
    for item_set in worn_valid_by_pid.values():
        pair_counts.update(itertools.combinations(sorted(item_set), 2))
    cooccurrence_pairs = []
    for (first, second), count in pair_counts.most_common(100):
        if first not in metrics or second not in metrics:
            continue
        cooccurrence_pairs.append({
            "vnum_a": first,
            "name_a": metrics[first]["name"],
            "vnum_b": second,
            "name_b": metrics[second]["name"],
            "players": count,
        })
    class_item_cores: dict[str, list[dict[str, Any]]] = {}
    for class_id, class_name in sorted(class_names.items()):
        class_pids = {pid for pid, ids in character_classes.items() if class_id in ids}
        core_rows: list[dict[str, Any]] = []
        for vnum, item_metric in metrics.items():
            support = len(item_metric["worn_players"] & class_pids)
            if not support or item_metric["exclusion_reasons"]:
                continue
            core_rows.append({
                "vnum": vnum,
                "name": item_metric["name"],
                "players": support,
                "share": round(support / len(class_pids), 4) if class_pids else 0.0,
                "slots": item_metric["slots"],
                "power_score": item_metric["power_score"],
                "risk_score": item_metric["risk_score"],
                "reason": item_metric["reason"],
            })
        core_rows.sort(key=lambda row: (row["players"], row["share"], -row["risk_score"], row["vnum"]), reverse=True)
        class_item_cores[class_name] = core_rows[:20]

    fundamentals = choose_fundamentals(area_objects, metrics, defines, class_rows, playable_races)
    recommendations = {
        "standard": build_recommendations(metrics, character_classes, class_rows, "standard"),
        "enhanceable": build_recommendations(metrics, character_classes, class_rows, "enhanceable"),
    }
    consumables = {
        "standard": summarize_consumable(metrics, area_objects, defines, "standard"),
        "enhanceable": summarize_consumable(metrics, area_objects, defines, "enhanceable"),
    }
    high_level_counts = {
        "threshold": args.level_threshold,
        "characters": cohort_size,
        "level_51_or_higher": sum(to_int(row["level"]) >= 51 for row in character_rows),
        "level_56_or_higher": sum(to_int(row["level"]) >= 56 for row in character_rows),
        "min_level": min((to_int(row["level"]) for row in character_rows), default=0),
        "max_level": max((to_int(row["level"]) for row in character_rows), default=0),
    }
    output = {
        "schema_version": 1,
        "source": {
            "database": args.database,
            "level_threshold": args.level_threshold,
            "area_root": args.area_root,
            "area_list": args.area_list,
            "enhance_config": args.enhance_config,
            "note": "aggregate/template-level output; player identifiers are intentionally omitted",
        },
        "cohort": high_level_counts,
        "class_counts": {
            class_names.get(class_id, str(class_id)): len(pids)
            for class_id, pids in sorted((class_id, {pid for pid, ids in character_classes.items() if class_id in ids}) for class_id in class_names)
        },
        "class_ids": {name: class_id for class_id, name in class_names.items()},
        "database_counts": {name: len(rows) for name, rows in sql.items()},
        "static_reconciliation": area_diagnostics,
        "enhance_config": enhance_config,
        "effect_prevalence": {
            "statuses": {
                status: {
                    "item_instances": worn_status_instances[status],
                    "players": len(worn_status_players[status]),
                }
                for status in sorted(worn_status_instances)
            },
            "affects": {
                affect: {
                    "item_instances": worn_affect_instances[affect],
                    "players": len(worn_affect_players[affect]),
                    "signed_modifier_total": worn_affect_totals[affect],
                }
                for affect in sorted(worn_affect_instances)
            },
        },
        "cooccurrence_pairs": cooccurrence_pairs,
        "class_item_cores": class_item_cores,
        "diagnostics": {
            **diagnostics,
            "missing_wiki_vnums": sorted(set(diagnostics["missing_wiki_vnums"])),
            "missing_area_vnums": sorted(set(diagnostics["missing_area_vnums"])),
            "exclusion_counts": dict(sorted(exclusion_counts.items())),
        },
        "fundamentals": fundamentals,
        "consumables": consumables,
        "recommendations": recommendations,
        "candidates": serializable_metrics,
    }
    (output_dir / "analysis.json").write_text(json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    csv_rows = []
    for metric in serializable_metrics:
        csv_rows.append({
            "vnum": metric["vnum"], "name": metric["name"], "type": metric["type_name"],
            "observed_players": metric["observed_players"], "observed_share": metric["observed_share"],
            "worn_players": metric["worn_players_count"], "carried_players": metric["carried_players_count"],
            "slots": json.dumps(metric["slots"], sort_keys=True), "base_exclusions": ";".join(metric["exclusion_reasons"]),
            "enhanceable": int(metric["enhanceable"]), "enhance_rejections": ";".join(metric["enhance_rejections"]),
            "power_score": metric["power_score"], "risk_score": metric["risk_score"],
            "reason": metric["reason"], "statuses": ";".join(metric["effect_summary"]["statuses"]),
            "affects": json.dumps(metric["effect_summary"]["affects"], sort_keys=True),
        })
    write_csv(output_dir / "candidates.csv", csv_rows, list(csv_rows[0]) if csv_rows else ["vnum", "name"])
    recommendation_rows = []
    for profile, by_class in recommendations.items():
        for class_name, entries in by_class.items():
            for entry in entries:
                recommendation_rows.append({"profile": profile, "class": class_name, **entry, "effect_summary": json.dumps(entry["effect_summary"], sort_keys=True)})
    write_csv(output_dir / "recommendations.csv", recommendation_rows, list(recommendation_rows[0]) if recommendation_rows else ["profile", "class", "slot", "vnum"])
    write_csv(output_dir / "fundamentals.csv", fundamentals, list(fundamentals[0]) if fundamentals else ["role", "vnum"])
    consumable_rows = []
    for profile, entries in consumables.items():
        for entry in entries:
            consumable_rows.append({"profile": profile, **entry})
    write_csv(output_dir / "consumables.csv", consumable_rows, list(consumable_rows[0]) if consumable_rows else ["profile", "category", "vnum"])
    return output


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--docker-container")
    parser.add_argument("--mysql-host")
    parser.add_argument("--mysql-port", type=int, default=3306)
    parser.add_argument("--mysql-user", default="duris")
    parser.add_argument("--mysql-password-env", default="DB_PASSWD")
    parser.add_argument("--database", default="duris_prod")
    parser.add_argument("--level-threshold", type=int, default=50)
    parser.add_argument("--area-root", default="areas/obj")
    parser.add_argument("--area-list", default="areas/AREA")
    parser.add_argument("--enhance-config", default="lib/enhance.cfg")
    parser.add_argument("--output-dir", required=True)
    return parser


def main() -> int:
    args = make_parser().parse_args()
    try:
        output = analyze(args)
    except (OSError, RuntimeError, ValueError) as error:
        print(f"chaos_eq_analyze: ERROR: {error}", file=sys.stderr)
        return 2
    print(json.dumps({
        "analysis": str(Path(args.output_dir).resolve() / "analysis.json"),
        "cohort_characters": output["cohort"]["characters"],
        "level_51_or_higher": output["cohort"]["level_51_or_higher"],
        "level_56_or_higher": output["cohort"]["level_56_or_higher"],
        "candidate_templates": len(output["candidates"]),
        "standard_class_profiles": len(output["recommendations"]["standard"]),
        "enhanceable_class_profiles": len(output["recommendations"]["enhanceable"]),
        "fundamentals": len(output["fundamentals"]),
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
