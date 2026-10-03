"""Definition-1 shared-battle values and complete mutation packets.

This portable contract does not activate the native collector or a SQL record
kind. Complete packets are necessary but do not establish cross-packet source,
alias, ownership or outcome coverage. The future writer uses the same numeric
field identities; a battle identity must never be cast to a legacy encounter.
"""
from __future__ import annotations

from collections.abc import Mapping, Sequence

DEFINITION_VERSION = 1
MAX_ACTORS = 64
MAX_PACKET_FACTS = 65
MAX_MUTATION_FACTS = 4096
UINT64_MAX = (1 << 64) - 1
UINT32_MAX = (1 << 32) - 1
INT32_MAX = (1 << 31) - 1
GENERATION_TAG = 1 << 63
QUALITY_KNOWN = (1 << 10) - 1
QUALITY_CONTEXT_OVERFLOW = 1 << 1
QUALITY_QUEUE_DROP = 1 << 4
QUALITY_UNCLOSED_TAIL = 1 << 6
QUALITY_CLOCK_DISCONTINUITY = 1 << 7
QUALITY_CARDINALITY_OVERFLOW = 1 << 9
INCOMPLETE_GRAPH = (QUALITY_CONTEXT_OVERFLOW | QUALITY_QUEUE_DROP |
                    QUALITY_CLOCK_DISCONTINUITY | QUALITY_CARDINALITY_OVERFLOW)

# Ordering, widths and signedness are immutable for this definition. The native
# telemetry_battle_fields.inc descriptor is checked against this tuple in tests.
FIELD_LAYOUT = (
    ("battle_boot_id", 8, False),
    ("battle_process_id", 8, False),
    ("battle_seq", 8, False),
    ("battle_related_boot_id", 8, False),
    ("battle_related_process_id", 8, False),
    ("battle_related_seq", 8, False),
    ("battle_environment_id", 8, False),
    ("battle_season_id", 8, False),
    ("battle_config_id", 8, False),
    ("battle_classifier_version", 4, False),
    ("battle_policy_version", 4, False),
    ("battle_scope_zone_vnum", 4, True),
    ("battle_scope_group_key", 8, False),
    ("battle_revision", 8, False),
    ("battle_fact_sequence", 4, False),
    ("battle_fact_index", 2, False),
    ("battle_fact_count", 2, False),
    ("battle_definition_version", 2, False),
    ("battle_fact_kind", 1, False),
    ("battle_relation", 1, False),
    ("battle_side_status", 1, False),
    ("battle_mode", 1, False),
    ("battle_close_reason", 1, False),
    ("battle_actor_active", 1, False),
    ("battle_actor_side", 1, False),
    ("battle_end_censored", 1, False),
    ("battle_actor_roles", 2, False),
    ("battle_actor_count", 2, False),
    ("battle_active_actor_count", 2, False),
    ("battle_observed_owner_count", 2, False),
    ("battle_dropped_actor_count", 4, False),
    ("battle_actor_id", 8, False),
    ("battle_actor_pid", 4, True),
    ("battle_actor_owner_subject_id", 8, False),
    ("battle_actor_kind", 1, False),
    ("battle_actor_power_band", 2, False),
    ("battle_actor_encounter_boot_id", 8, False),
    ("battle_actor_encounter_process_id", 8, False),
    ("battle_actor_encounter_seq", 8, False),
    ("battle_actor_session_boot_id", 8, False),
    ("battle_actor_session_process_id", 8, False),
    ("battle_actor_session_seq", 8, False),
    ("battle_actor_level_band", 2, False),
    ("battle_actor_class_id", 2, False),
    ("battle_actor_race_id", 2, False),
    ("battle_actor_faction_id", 2, False),
    ("battle_actor_zone_vnum", 4, True),
    ("battle_actor_group_size", 4, False),
    ("battle_actor_group_key", 8, False),
    ("battle_actor_group_revision", 2, False),
    ("battle_actor_context_version", 2, False),
    ("battle_actor_quality_flags", 4, False),
    ("battle_related_actor_id", 8, False),
    ("battle_related_actor_kind", 1, False),
    ("battle_present_usec", 8, False),
    ("battle_contributor_usec", 8, False),
    ("battle_pve_usec", 8, False),
    ("battle_pvp_usec", 8, False),
    ("battle_mixed_usec", 8, False),
    ("battle_unknown_mode_usec", 8, False),
    ("battle_outnumbered_owner_usec", 8, False),
    ("battle_unknown_side_usec", 8, False),
    ("battle_start_monotonic_usec", 8, False),
    ("battle_at_monotonic_usec", 8, False),
    ("battle_observed_through_monotonic_usec", 8, False),
    ("battle_last_engagement_monotonic_usec", 8, False),
    ("battle_inactivity_grace_usec", 8, False),
    ("battle_at_utc_usec", 8, True),
    ("battle_observed_through_utc_usec", 8, True),
    ("battle_quality_flags", 4, False),
)
FIELDS = tuple(name for name, _, _ in FIELD_LAYOUT)
WIRE_BYTES = sum(width for _, width, _ in FIELD_LAYOUT)
BATTLE = ("battle_boot_id", "battle_process_id", "battle_seq")
RELATED_BATTLE = ("battle_related_boot_id", "battle_related_process_id", "battle_related_seq")
ACTOR_VALUES = tuple(name for name in FIELDS if name.startswith("battle_actor_") and name not in (
    "battle_actor_active", "battle_actor_side", "battle_actor_roles", "battle_actor_count"))
EFFORT = tuple("battle_" + name + "_usec" for name in (
    "present", "contributor", "pve", "pvp", "mixed", "unknown_mode", "outnumbered_owner", "unknown_side"))
PACKET_HEADER = BATTLE + tuple("battle_" + name for name in (
    "environment_id", "season_id", "config_id", "classifier_version", "policy_version",
    "scope_zone_vnum", "scope_group_key", "revision", "fact_count", "definition_version",
    "mode", "close_reason", "end_censored", "actor_count", "active_actor_count",
    "observed_owner_count", "dropped_actor_count", "start_monotonic_usec", "at_monotonic_usec",
    "observed_through_monotonic_usec", "last_engagement_monotonic_usec", "inactivity_grace_usec",
    "at_utc_usec", "observed_through_utc_usec"))


class BattleContractError(ValueError):
    pass


def _require(condition: bool, reason: str) -> None:
    if not condition:
        raise BattleContractError(reason)


def battle_id(row: Mapping[str, int]) -> tuple[int, int, int]:
    return tuple(row[name] for name in BATTLE)


def fact_key(row: Mapping[str, int]) -> tuple[int, int, int, int]:
    return (*battle_id(row), row["battle_fact_sequence"])


def _actor_key_valid(actor: int, kind: int) -> bool:
    return (0 < actor <= INT32_MAX if kind == 1 else
            kind in (2, 3) and bool(actor & GENERATION_TAG) and bool(actor & ~GENERATION_TAG))


def _optional_id(row: Mapping[str, int], names: tuple[str, ...]) -> bool:
    values = tuple(row[name] for name in names)
    return all(value == 0 for value in values) or all(value > 0 for value in values)


def validate_fact(row: Mapping[str, int]) -> dict[str, int]:
    _require(isinstance(row, Mapping) and set(row) == set(FIELDS), "exact battle field set required")
    for name, width, signed in FIELD_LAYOUT:
        value = row[name]
        limit = 1 << (width * 8 - int(signed))
        _require(type(value) is int and (-limit if signed else 0) <= value < limit,
                 "battle field width/type: " + name)
    value = dict(row)
    _require(all(value[name] > 0 for name in BATTLE), "battle producer/identity")
    _require(value["battle_definition_version"] == DEFINITION_VERSION, "battle definition")
    _require(all(value["battle_" + name] > 0 for name in (
        "environment_id", "season_id", "config_id", "classifier_version", "policy_version")) and
        value["battle_scope_zone_vnum"] == -1 and value["battle_scope_group_key"] == 0, "global battle scope")
    kind, index, count = (value["battle_" + name] for name in ("fact_kind", "fact_index", "fact_count"))
    _require(value["battle_revision"] > 0 and 0 <= index < count and value["battle_fact_sequence"] > index,
             "battle packet identity/ordinal")
    _require(1 <= kind <= 7 and 1 <= value["battle_side_status"] <= 3 and 0 <= value["battle_mode"] <= 3,
             "battle enum")
    quality = value["battle_quality_flags"]
    _require(quality & ~QUALITY_KNOWN == 0 and 2 <= value["battle_actor_count"] <= MAX_ACTORS and
             value["battle_observed_owner_count"] <= value["battle_active_actor_count"] <= value["battle_actor_count"],
             "battle quality/cardinality")
    start, engagement, through, at = (value["battle_" + name + "_monotonic_usec"] for name in (
        "start", "last_engagement", "observed_through", "at"))
    _require(start <= engagement <= through <= at and value["battle_inactivity_grace_usec"] > 0, "battle clocks")
    active, side, roles = (value["battle_actor_" + name] for name in ("active", "side", "roles"))
    _require(active <= 1 and side <= 2 and value["battle_end_censored"] <= 1 and roles & ~7 == 0, "battle actor flags")
    present = value["battle_present_usec"]
    _require(sum(value["battle_" + mode + "_usec"] for mode in ("pve", "pvp", "mixed", "unknown_mode")) == present and
             value["battle_contributor_usec"] <= present <= through - start and
             value["battle_outnumbered_owner_usec"] <= value["battle_pvp_usec"] and
             value["battle_unknown_side_usec"] <= present, "battle effort conservation")
    terminal = kind in (6, 7)
    if terminal:
        reason = value["battle_close_reason"]
        _require(1 <= reason <= 3 and value["battle_end_censored"] == 1 and quality & QUALITY_UNCLOSED_TAIL != 0 and
                 count == value["battle_actor_count"] + 1 and
                 (index == value["battle_actor_count"] if kind == 7 else index < value["battle_actor_count"]),
                 "censored battle closure")
        _require(at - engagement >= value["battle_inactivity_grace_usec"] if reason == 1 else
                 through == at and value["battle_observed_through_utc_usec"] == value["battle_at_utc_usec"],
                 "battle closure boundary")
    else:
        _require(value["battle_close_reason"] == 0 and value["battle_end_censored"] == 0 and count <= 5,
                 "battle mutation shape")
    if kind in (2, 3, 6):
        actor, actor_kind, owner, pid = (value["battle_actor_" + name] for name in ("id", "kind", "owner_subject_id", "pid"))
        actor_quality = value["battle_actor_quality_flags"]
        _require(_actor_key_valid(actor, actor_kind) and owner <= INT32_MAX and
                 (pid == actor and owner == actor if actor_kind == 1 else pid == -1 and
                  (owner > 0 if actor_kind == 2 else owner == 0)), "battle actor generation/owner")
        _require(value["battle_actor_context_version"] == 1 and value["battle_actor_zone_vnum"] >= -1 and
                 actor_quality & ~QUALITY_KNOWN == 0 and actor_quality & ~quality == 0 and roles > 0 and
                 (active != 0 or side == 0) and (value["battle_side_status"] == 1 or side == 0) and
                 (roles & 3 != 0 or value["battle_contributor_usec"] == 0), "battle actor context/roles")
        group = value["battle_actor_group_key"]
        _require((bool(group & ~GENERATION_TAG) and value["battle_actor_group_revision"] > 0) if group & GENERATION_TAG else
                 value["battle_actor_group_revision"] == 0, "battle formal roster revision")
        encounter = tuple("battle_actor_encounter_" + name for name in ("boot_id", "process_id", "seq"))
        session = tuple("battle_actor_session_" + name for name in ("boot_id", "process_id", "seq"))
        _require(_optional_id(value, encounter) and _optional_id(value, session), "battle optional linkage")
        _require(value[encounter[2]] == 0 or tuple(value[name] for name in encounter[:2]) == battle_id(value)[:2],
                 "battle encounter producer")
        _require(actor_kind == 1 or all(value[name] == 0 for name in session), "battle logical session belongs to a PC")
    else:
        _require(all(value[name] == 0 for name in ACTOR_VALUES) and active == 0 and side == 0 and roles == 0 and present == 0,
                 "absent battle actor")
    if kind == 5:
        _require(all(value[name] > 0 for name in RELATED_BATTLE) and
                 tuple(value[name] for name in RELATED_BATTLE[:2]) == battle_id(value)[:2] and
                 value[RELATED_BATTLE[2]] > value["battle_seq"] and index == 0, "battle canonical alias")
    else:
        _require(all(value[name] == 0 for name in RELATED_BATTLE), "absent battle alias")
    if kind == 3:
        relation = value["battle_relation"]
        _require(1 <= relation <= 3 and active == 1 and _actor_key_valid(value["battle_related_actor_id"], value["battle_related_actor_kind"]) and
                 (relation == 2 or value["battle_related_actor_id"] != value["battle_actor_id"]), "battle relation")
    else:
        _require(value["battle_relation"] == 0 and value["battle_related_actor_id"] == 0 and value["battle_related_actor_kind"] == 0,
                 "absent battle relation")
    _require(kind != 1 or index == 0 and value["battle_revision"] == 1 and count == 5, "battle start ordinal")
    _require(kind != 4 or index + 1 == count, "battle final cut")
    _require(kind != 2 or index + 1 < count, "battle context before cut")
    _require(value["battle_dropped_actor_count"] == 0 or quality & QUALITY_CARDINALITY_OVERFLOW != 0, "battle dropped actor evidence")
    _require(quality & INCOMPLETE_GRAPH == 0 or value["battle_side_status"] != 1, "incomplete graph cannot qualify sides")
    return value


def encode_fact(row: Mapping[str, int]) -> bytes:
    value = validate_fact(row)
    return b"".join(value[name].to_bytes(width, "big", signed=signed) for name, width, signed in FIELD_LAYOUT)


def decode_fact(wire: bytes) -> dict[str, int]:
    _require(isinstance(wire, (bytes, bytearray)) and len(wire) == WIRE_BYTES, "exact battle wire length required")
    value, offset = {}, 0
    for name, width, signed in FIELD_LAYOUT:
        value[name] = int.from_bytes(wire[offset:offset + width], "big", signed=signed)
        offset += width
    return validate_fact(value)


def _same_actor_values(a: Mapping[str, int], b: Mapping[str, int]) -> bool:
    names = tuple(name for name in FIELDS if name.startswith("battle_actor_") and name != "battle_actor_side") + EFFORT
    return all(a[name] == b[name] for name in names) and (a["battle_side_status"] != b["battle_side_status"] or
                                                         a["battle_actor_side"] == b["battle_actor_side"])


def validate_packet(rows: Sequence[Mapping[str, int]]) -> tuple[dict[str, int], ...]:
    _require(isinstance(rows, (tuple, list)) and 0 < len(rows) <= MAX_PACKET_FACTS, "bounded battle packet required")
    values = tuple(validate_fact(row) for row in rows)
    first, last, count = values[0], values[-1], len(values)
    _require(first["battle_fact_count"] == count and first["battle_fact_sequence"] <= UINT32_MAX - count + 1, "complete battle packet length")
    previous_quality = 0
    for index, row in enumerate(values):
        _require(all(row[name] == first[name] for name in PACKET_HEADER) and row["battle_fact_index"] == index and
                 row["battle_fact_sequence"] == first["battle_fact_sequence"] + index and
                 previous_quality & ~row["battle_quality_flags"] == 0, "battle packet header/order/loss")
        previous_quality = row["battle_quality_flags"]
        if row["battle_fact_kind"] == 3:
            for actor in values[:index]:
                if actor["battle_fact_kind"] == 2:
                    _require(row["battle_actor_id"] != actor["battle_actor_id"] or _same_actor_values(row, actor), "conflicting battle actor snapshot")
                    _require(row["battle_related_actor_id"] != actor["battle_actor_id"] or
                             row["battle_related_actor_kind"] == actor["battle_actor_kind"], "conflicting battle target kind")
    kinds = tuple(row["battle_fact_kind"] for row in values)
    if kinds[-1] == 7:
        _require(all(kind == 6 for kind in kinds[:-1]) and len({row["battle_actor_id"] for row in values[:-1]}) == count - 1,
                 "unique terminal battle roster")
        active = tuple(row for row in values[:-1] if row["battle_actor_active"])
        _require(len(active) == last["battle_active_actor_count"] and
                 len({row["battle_actor_owner_subject_id"] for row in active if row["battle_actor_owner_subject_id"]}) == last["battle_observed_owner_count"],
                 "terminal battle owner/actor counts")
        return values
    _require(kinds[-1] == 4, "final battle cut required")
    if kinds[0] == 1:
        _require(kinds == (1, 2, 2, 3, 4) and first["battle_fact_sequence"] == 1 and first["battle_actor_count"] == 2 and
                 first["battle_active_actor_count"] == 2 and first["battle_observed_owner_count"] > 0 and
                 first["battle_start_monotonic_usec"] == first["battle_at_monotonic_usec"], "initial battle packet")
        a, b, relation = values[1:4]
        _require(a["battle_actor_id"] != b["battle_actor_id"] and a["battle_actor_active"] == b["battle_actor_active"] == 1 and
                 a["battle_actor_roles"] == b["battle_actor_roles"] == 1 and a["battle_present_usec"] == b["battle_present_usec"] == 0 and
                 relation["battle_relation"] == 1 and relation["battle_actor_id"] == a["battle_actor_id"] and
                 relation["battle_related_actor_id"] == b["battle_actor_id"] and relation["battle_related_actor_kind"] == b["battle_actor_kind"],
                 "initial battle hostile pair")
        owners = {row["battle_actor_owner_subject_id"] for row in (a, b) if row["battle_actor_owner_subject_id"]}
        _require(len(owners) == first["battle_observed_owner_count"] and
                 first["battle_mode"] == (2 if a["battle_actor_owner_subject_id"] and b["battle_actor_owner_subject_id"] else 1),
                 "initial battle mode/owners")
        return values
    _require(first["battle_revision"] > 1, "initial battle start missing")
    if count == 1:
        _require(last["battle_quality_flags"] & QUALITY_CONTEXT_OVERFLOW != 0, "single overflow cut")
        return values
    alias, contexts, relations = kinds[0] == 5, [], 0
    for index in range(int(alias), count - 1):
        row = values[index]
        if row["battle_fact_kind"] == 2:
            _require(relations == 0 and len(contexts) < 2 and row["battle_actor_id"] not in contexts, "unique ordered mutation contexts")
            contexts.append(row["battle_actor_id"])
        elif row["battle_fact_kind"] == 3:
            relations += 1
            _require(relations == 1 and index + 2 == count and
                     (not alias or row["battle_actor_id"] != row["battle_related_actor_id"]), "one final mutation relation")
        else:
            raise BattleContractError("unexpected mutation battle fact")
    _require(relations == 1 if alias else bool(contexts or relations), "battle mutation/alias evidence")
    return values


class PacketBuffer:
    """One bounded packet; explicit reset retains loss and conflict visibility."""

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self.identity = None
        self.first_sequence = self.count = 0
        self.complete = self.conflicted = False
        self._rows: dict[int, dict[str, int]] = {}

    @property
    def received(self) -> int:
        return len(self._rows)

    def receive(self, row: Mapping[str, int]) -> str:
        try:
            value = validate_fact(row)
        except BattleContractError:
            return "invalid"
        if self.conflicted:
            return "duplicate_conflict"
        identity = (*battle_id(value), value["battle_revision"])
        sequence = value["battle_fact_sequence"] - value["battle_fact_index"]
        if self.identity is None:
            self.identity, self.first_sequence, self.count = identity, sequence, value["battle_fact_count"]
        elif self.identity != identity:
            return "other_packet"
        if sequence != self.first_sequence or value["battle_fact_count"] != self.count:
            self.conflicted, self.complete = True, False
            return "duplicate_conflict"
        index = value["battle_fact_index"]
        if index in self._rows:
            if self._rows[index] == value:
                return "duplicate_identical"
            self.conflicted, self.complete = True, False
            return "duplicate_conflict"
        self._rows[index] = value
        if self.received < self.count:
            return "pending"
        try:
            validate_packet([self._rows[index] for index in range(self.count)])
        except BattleContractError:
            self.conflicted = True
            return "invalid"
        self.complete = True
        return "complete"

    def packet(self) -> tuple[dict[str, int], ...]:
        _require(self.complete and not self.conflicted, "complete unconflicted battle packet required")
        return tuple(dict(self._rows[index]) for index in range(self.count))
