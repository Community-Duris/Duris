"""Bounded shared-battle history and contribution linkage for publication.

Individual immutable facts are not a complete history. This reducer uses the
sealed family validators, retains missing packets and lifecycles as uncertainty,
and resolves only complete observed merge packets. It performs no database I/O
and does not establish winners, human identity or complete contribution coverage.
"""
from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
import hashlib
from typing import Any, Callable, Mapping, Sequence

try:
    from . import battle_contract as battle, battle_contribution_contract as contribution, battle_build_contract as builds, control_contract as controls, incident
    from .rollup_definitions import (ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_INCIDENT_GAP, ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN,
        ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_BACKWARD, ROLLUP_QUALITY_UTC_MISMATCH)
except ImportError:
    import battle_contract as battle
    import battle_contribution_contract as contribution
    import battle_build_contract as builds
    import control_contract as controls
    import incident
    from rollup_definitions import (ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_INCIDENT_GAP, ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN,
        ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_BACKWARD, ROLLUP_QUALITY_UTC_MISMATCH)

MAX_INPUTS = 16_384
MAX_BATTLES = 512
MAX_PACKETS = 4_096
MAX_REFERENCE_SNAPSHOTS = 2_048
INPUT_BYTE_BOUND = 8_192
ACTOR_BYTE_BOUND = 2_048
OUTPUT_BYTE_BOUND = 4_096
DEFAULT_BYTE_LIMIT = 32 * 1024 * 1024
HEADER = ("boot_id", "process_id", "record_seq", "schema_version", "record_kind", "occurrence_utc_usec")
ACTOR_CONTEXT = battle.ACTOR_VALUES
COUNTERS = tuple("bc_" + name for _bit, names in contribution.COUNTER_FAMILIES for name in names)
SCOPE = tuple("battle_" + name for name in (
    "environment_id", "season_id", "config_id", "classifier_version", "policy_version",
    "scope_zone_vnum", "scope_group_key"))
EXPOSURE_CONTEXT = (*SCOPE, *ACTOR_CONTEXT, "battle_actor_roles", "battle_actor_side", "battle_mode",
    "battle_side_status", "observed_side_owners", "observed_opposing_owners", "observed_roster_digest", "quality_flags")


class HistoryError(ValueError):
    """A payload-free conflicting source or bounded history failure."""


def _require(condition: bool, reason: str) -> None:
    if not condition:
        raise HistoryError(reason)


def _integer(value: Any, *, lower=0, upper=battle.UINT64_MAX) -> bool:
    return type(value) is int and lower <= value <= upper


@dataclass(slots=True)
class _Budget:
    limit: int
    max_output: int
    deadline: Callable[[], None]
    used: int = 0
    outputs: int = 0

    def reserve(self, amount: int) -> None:
        self.deadline()
        _require(amount <= self.limit - self.used, "history_byte_capacity")
        self.used += amount

    def output(self) -> None:
        _require(self.outputs < self.max_output, "history_output_capacity")
        self.reserve(OUTPUT_BYTE_BOUND)
        self.outputs += 1


@dataclass(slots=True)
class _Actor:
    context: dict[str, int]
    active: int
    roles: int
    side: int
    effort: dict[str, int]

    @classmethod
    def from_fact(cls, row: Mapping[str, int]) -> _Actor:
        return cls({name: row[name] for name in ACTOR_CONTEXT}, row["battle_actor_active"],
            row["battle_actor_roles"], row["battle_actor_side"], {name: row[name] for name in battle.EFFORT})

    def clone(self) -> _Actor:
        return _Actor(dict(self.context), self.active, self.roles, self.side, dict(self.effort))


@dataclass(slots=True)
class _State:
    identity: tuple[int, int, int]
    actors: dict[int, _Actor] = field(default_factory=dict)
    edges: dict[int, set[tuple[int, int]]] = field(default_factory=lambda: {1: set(), 2: set(), 3: set()})
    last: dict[str, int] | None = None
    cut: int | None = None
    cut_utc: int | None = None
    exposures: list[dict[str, Any]] = field(default_factory=list)
    last_exposure: dict[int, dict[str, Any]] = field(default_factory=dict)
    revision: int = 0
    last_sequence: int = 0
    quality: int = 0
    integrity: bool = True
    replay_verified: bool = True
    start_seen: bool = False
    close_seen: bool = False
    retired: bool = False
    packets: int = 0
    incomplete_packets: int = 0
    source_facts: int = 0


@dataclass(frozen=True, slots=True)
class BattleHistory:
    """Value output; alternative projections must not be summed together."""
    summary: Mapping[str, Any]
    battles: tuple[Mapping[str, Any], ...]
    actors: tuple[Mapping[str, Any], ...]
    contributions: tuple[Mapping[str, Any], ...]
    exposures: tuple[Mapping[str, Any], ...]
    builds: tuple[Mapping[str, Any], ...] = ()
    controls: tuple[Mapping[str, Any], ...] = ()


def _group(actor: _Actor) -> bool:
    key = actor.context["battle_actor_group_key"]
    return bool(key & battle.GENERATION_TAG and key & ~battle.GENERATION_TAG and
                actor.context["battle_actor_group_revision"])


def _classify(state: _State, quality: int) -> tuple[int, int]:
    """Review the bounded observed constraints, never invent coalition membership."""
    active = sorted(actor_id for actor_id, actor in state.actors.items() if actor.active)
    for actor in state.actors.values():
        actor.side = 0
    contradictory = stale = False
    components = 0
    for root in active:
        if state.actors[root].side:
            continue
        components += 1
        state.actors[root].side = 1
        pending = [root]
        for source in pending:
            a = state.actors[source]
            for target in active:
                if source == target:
                    continue
                b = state.actors[target]
                same_group = (_group(a) and a.context["battle_actor_group_key"] == b.context["battle_actor_group_key"] and
                              a.context["battle_actor_group_revision"] == b.context["battle_actor_group_revision"])
                stale |= (_group(a) and _group(b) and a.context["battle_actor_group_key"] == b.context["battle_actor_group_key"] and
                          a.context["battle_actor_group_revision"] != b.context["battle_actor_group_revision"])
                pair = tuple(sorted((source, target)))
                hostile = pair in state.edges[1]
                friendly = (pair in state.edges[2] or same_group or
                    (a.context["battle_actor_owner_subject_id"] != 0 and
                     a.context["battle_actor_owner_subject_id"] == b.context["battle_actor_owner_subject_id"]))
                if not hostile and not friendly:
                    continue
                contradictory |= hostile and friendly
                expected = 3 - a.side if hostile else a.side
                if b.side == 0:
                    b.side = expected
                    pending.append(target)
                else:
                    contradictory |= b.side != expected
    status = 2 if contradictory else 3 if stale or components > 1 or quality & battle.INCOMPLETE_GRAPH else 1
    if status != 1:
        for actor in state.actors.values():
            actor.side = 0
    pvp = pve = False
    for first, second in state.edges[1]:
        if first in state.actors and second in state.actors and state.actors[first].active and state.actors[second].active:
            owned = bool(state.actors[first].context["battle_actor_owner_subject_id"] and
                         state.actors[second].context["battle_actor_owner_subject_id"])
            pvp |= owned
            pve |= not owned
    return status, 3 if pvp and pve else 2 if pvp else 1 if pve else 0


def _roster_digest(state: _State) -> str:
    """Commit the observed active composition and relationships, not a census."""
    digest = hashlib.sha256(b"duris-battle-exposure-roster-v1:")
    active = {actor_id for actor_id, actor in state.actors.items() if actor.active}
    for actor_id in sorted(active):
        actor = state.actors[actor_id]
        digest.update(b"A")
        for value in (*tuple(actor.context[name] for name in ACTOR_CONTEXT), actor.roles, actor.side):
            digest.update(value.to_bytes(9, "big", signed=True))
    for relation, edges in sorted(state.edges.items()):
        for first, last in sorted(edges):
            if first in active and last in active:
                digest.update(b"E" + bytes((relation,)) + first.to_bytes(8, "big") + last.to_bytes(8, "big"))
    return digest.hexdigest()


def _seal(state: _State, boundary: Mapping[str, int], budget: _Budget, *, terminal: bool = False) -> None:
    clock = "observed_through" if terminal else "at"
    at = boundary["battle_" + clock + "_monotonic_usec"]
    utc = boundary["battle_" + clock + "_utc_usec"]
    if state.cut is None:
        state.cut = at
        state.cut_utc = utc
        return
    _require(at >= state.cut, "history_observed_clock_reversal")
    elapsed = at - state.cut
    if elapsed and state.replay_verified and state.last is not None:
        owners = {side: {actor.context["battle_actor_owner_subject_id"] for actor in state.actors.values()
            if actor.active and actor.side == side and actor.context["battle_actor_owner_subject_id"]} for side in (1, 2)}
        mode = state.last["battle_mode"]
        roster_digest = _roster_digest(state)
        for actor in state.actors.values():
            if not actor.active:
                continue
            additions = {"battle_present_usec": elapsed,
                "battle_contributor_usec": elapsed if actor.roles & 3 else 0,
                "battle_" + ("unknown_mode", "pve", "pvp", "mixed")[mode] + "_usec": elapsed,
                "battle_unknown_side_usec": elapsed if state.last["battle_side_status"] != 1 else 0,
                "battle_outnumbered_owner_usec": elapsed if mode == 2 and actor.context["battle_actor_owner_subject_id"] and
                    actor.side in (1, 2) and len(owners[actor.side]) < len(owners[3 - actor.side]) else 0}
            quality = state.quality | _utc_quality((state.cut_utc, utc), state.quality)
            if state.cut_utc != contribution.UTC_UNKNOWN and utc != contribution.UTC_UNKNOWN and utc - state.cut_utc != elapsed:
                quality |= ROLLUP_QUALITY_UTC_MISMATCH
            exposure = {"source_battle": state.identity,
                "start_association": (*state.identity, state.last["battle_revision"], state.last["battle_fact_sequence"]),
                "through_association": (*battle.battle_id(boundary), boundary["battle_revision"], boundary["battle_fact_sequence"]),
                "start_record_seq": state.last["record_seq"], "through_record_seq": boundary["record_seq"],
                **{name: state.last[name] for name in SCOPE}, **actor.context,
                "battle_actor_roles": actor.roles, "battle_actor_side": actor.side,
                "battle_mode": mode, "battle_side_status": state.last["battle_side_status"],
                "start_monotonic_usec": state.cut, "observed_through_monotonic_usec": at,
                "start_utc_usec": state.cut_utc, "observed_through_utc_usec": utc,
                "observed_side_owners": len(owners[actor.side]) if actor.side in (1, 2) else None,
                "observed_opposing_owners": len(owners[3 - actor.side]) if actor.side in (1, 2) else None,
                "observed_roster_digest": roster_digest,
                **{name: additions.get(name, 0) for name in battle.EFFORT},
                "quality_flags": quality, "history_verified": True,
                "complete_population_coverage_implied": False}
            previous = state.last_exposure.get(actor.context["battle_actor_id"])
            if previous is not None and previous["observed_through_monotonic_usec"] == state.cut and all(
                    previous[name] == exposure[name] for name in EXPOSURE_CONTEXT):
                for name in battle.EFFORT:
                    _require(exposure[name] <= battle.UINT64_MAX - previous[name], "history_exposure_capacity")
                    previous[name] += exposure[name]
                previous.update({name: exposure[name] for name in (
                    "through_association", "through_record_seq", "observed_through_monotonic_usec", "observed_through_utc_usec")})
            else:
                budget.output()
                state.exposures.append(exposure)
                state.last_exposure[actor.context["battle_actor_id"]] = exposure
            for name, amount in additions.items():
                _require(amount <= battle.UINT64_MAX - actor.effort[name], "history_effort_capacity")
                actor.effort[name] += amount
    state.cut = at
    state.cut_utc = utc


def _accept_actor(state: _State, row: Mapping[str, int], trusted: bool, budget: _Budget) -> None:
    actor_id = row["battle_actor_id"]
    current = state.actors.get(actor_id)
    incoming = _Actor.from_fact(row)
    if current is None:
        _require(len(state.actors) < battle.MAX_ACTORS, "history_actor_capacity")
        budget.reserve(ACTOR_BYTE_BOUND)
        if trusted:
            _require(not any(incoming.effort.values()), "history_new_actor_inherits_effort")
    elif trusted:
        _require(all(incoming.effort[name] == current.effort[name] for name in battle.EFFORT),
                 "history_cumulative_effort_conflict")
        _require(current.roles & ~incoming.roles == 0, "history_roles_regressed")
    state.actors[actor_id] = incoming


def _canonical(identity: tuple[int, int, int], aliases: Mapping[tuple[int, int, int], tuple[int, int, int]]) -> tuple[int, int, int]:
    visited = set()
    while identity in aliases:
        _require(identity not in visited, "history_alias_cycle")
        visited.add(identity)
        identity = aliases[identity]
    return identity


def _signature(cut: Mapping[str, int], actor: _Actor) -> tuple[int, ...]:
    return (*tuple(cut[name] for name in SCOPE), *tuple(actor.context[name] for name in ACTOR_CONTEXT),
            cut["battle_mode"], cut["battle_side_status"], actor.side, cut["battle_quality_flags"])


def _utc_quality(points: Sequence[int], raw_quality: int) -> int:
    quality = ROLLUP_QUALITY_UTC_UNKNOWN if contribution.UTC_UNKNOWN in points or raw_quality & battle.QUALITY_CLOCK_DISCONTINUITY else 0
    known = [point for point in points if point != contribution.UTC_UNKNOWN]
    if any(first > last for first, last in zip(known, known[1:])):
        quality |= ROLLUP_QUALITY_UTC_BACKWARD
    return quality


def _coverage_quality(coverage: Mapping[str, Any] | None, producer: tuple[int, int], kind: int,
                      first: int | None, last: int | None, first_seq: int, last_seq: int, *, schema_version=4) -> int:
    if coverage is None or coverage.get("status") in ("not_published", "not_registered"):
        return ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN
    _require(coverage.get("registry_schema_version") == schema_version and coverage.get("status") == "reviewed_inventory",
             "history_incident_schema_or_status")
    rows = coverage.get("incidents")
    _require(isinstance(rows, (tuple, list)) and len(rows) <= incident.MAX_INCIDENTS, "history_incident_capacity")
    quality = 0
    reviewed_first, reviewed_last = coverage.get("reviewed_from_utc_usec"), coverage.get("reviewed_through_utc_usec")
    try:
        incident._range(incident._utc(reviewed_first), incident._utc(reviewed_last))
    except incident.IncidentError as error:
        raise HistoryError("history_incident_review_range") from error
    if first is None or last is None or reviewed_first is None or reviewed_last is None or reviewed_first > first or reviewed_last < last:
        quality |= ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN
    for row in rows:
        _require(isinstance(row, Mapping) and row.get("status") in ("active", "withdrawn") and
                 _integer(row.get("record_kind_mask"), lower=1, upper=incident.schema_contract(schema_version)[0]) and row["record_kind_mask"] & 1 == 0,
                 "history_incident_fields")
        if row["status"] != "active" or not row["record_kind_mask"] & (1 << kind):
            continue
        owner = row.get("producer_boot_id"), row.get("producer_process_id")
        _require(owner == (None, None) or all(_integer(part, lower=1) for part in owner), "history_incident_producer")
        if owner != (None, None) and owner != producer:
            continue
        begin, end = row.get("start_utc_usec"), row.get("end_utc_usec")
        low, high = row.get("first_record_seq"), row.get("last_record_seq")
        try:
            incident._range(incident._utc(begin), incident._utc(end))
            incident._range(incident._integer(low, nullable=True), incident._integer(high, nullable=True))
        except incident.IncidentError as error:
            raise HistoryError("history_incident_ranges") from error
        # A known occurrence bound can prove no overlap. Sequence bounds can do
        # so only for the explicitly matching producer; unknown/global incidents
        # cannot borrow another producer's sequence namespace.
        if first is not None and last is not None and ((begin is not None and begin > last) or (end is not None and end < first)):
            continue
        if owner == producer and ((low is not None and low > last_seq) or (high is not None and high < first_seq)):
            continue
        quality |= ROLLUP_QUALITY_INCIDENT_GAP
    return quality


def build_history(rows: Sequence[Mapping[str, Any]], scope: tuple[int, int], *,
                  incident_coverage: Mapping[str, Any] | None = None, incident_schema_version: int = 4, max_output_rows: int = 2_000,
                  max_total_bytes: int = DEFAULT_BYTE_LIMIT,
                  check_deadline: Callable[[], None] = lambda: None) -> BattleHistory:
    """Reduce an exact bounded source window, including partial histories.

    Inputs are canonical raw kind-10/11 rows in one environment/season. Transport
    ordering is producer scoped and independent of SQL arrival order. Identical
    receipt retries count once; a second receipt for one logical identity is a
    conflict. Missing packet/alias/actor evidence never becomes a qualified link.
    """
    _require(type(scope) is tuple and len(scope) == 2 and all(_integer(part, lower=1) for part in scope), "history_scope")
    _require(isinstance(rows, (tuple, list)) and len(rows) <= MAX_INPUTS, "history_input_capacity")
    _require(type(incident_schema_version) is int and incident_schema_version in (4, 5, 6), "history_incident_version")
    _require(_integer(max_output_rows, lower=1) and _integer(max_total_bytes, lower=1) and callable(check_deadline), "history_bounds")
    budget = _Budget(max_total_bytes, max_output_rows, check_deadline)
    facts: dict[tuple[int, int, int, int], dict[str, int]] = {}
    segments: dict[tuple[int, int, int], dict[str, int]] = {}
    build_points = {}
    control_points = {}
    receipts: dict[tuple[int, int, int], dict[str, int]] = {}
    requested = set()
    wanted_actors: dict[tuple[int, int, int], set[int]] = defaultdict(set)
    retries = 0
    for raw in rows:
        budget.reserve(INPUT_BYTE_BOUND)
        _require(isinstance(raw, Mapping) and raw.get("record_kind") in ((10, 11, 12, 13) if incident_schema_version == 6 else
            (10, 11, 12) if incident_schema_version == 5 else (10, 11)),
            "history_source_family")
        try:
            value = (battle.validate_raw_fact(raw) if raw["record_kind"] == 10 else
                contribution.validate_raw_segment(raw) if raw["record_kind"] == 11 else
                controls.validate_raw_observation(raw) if raw["record_kind"] == 13 else builds.validate_raw_observation(raw))
        except (battle.BattleContractError, contribution.ContributionContractError, builds.BuildContractError, controls.ControlContractError) as error:
            raise HistoryError(str(error)) from error
        prefix = {10: "battle_", 11: "bc_", 12: "bctx_", 13: "ctl_"}[raw["record_kind"]]
        _require((value[prefix + "environment_id"], value[prefix + "season_id"]) == scope, "history_source_scope")
        retained = dict(value, **{name: raw[name] for name in HEADER})
        receipt = tuple(raw[name] for name in HEADER[:3])
        if receipt in receipts:
            _require(receipts[receipt] == retained, "history_transport_conflict")
            retries += 1
            continue
        receipts[receipt] = retained
        target, key = ((facts, battle.fact_key(value)) if raw["record_kind"] == 10 else
            (segments, contribution.segment_key(value)) if raw["record_kind"] == 11 else
            (control_points, controls.observation_key(value)) if raw["record_kind"] == 13 else (build_points, builds.observation_key(value)))
        _require(key not in target, "history_logical_receipt_conflict")
        target[key] = retained
        if raw["record_kind"] == 13:
            for actor_prefix in ("source", "target"):
                reference = controls._association(value, actor_prefix)
                if any(reference):
                    identity = (*key[:2], reference[0])
                    wanted_actors[identity].add(value["ctl_" + actor_prefix + "_actor_id"])
                    requested.add((*identity, *reference[1:]))
                    if actor_prefix == "target":
                        requested.add((*identity, value["ctl_last_target_association_revision"],
                            value["ctl_last_target_association_fact_sequence"]))
        if raw["record_kind"] == 12:
            identity = tuple(value[name] for name in builds.BATTLE)
            wanted_actors[identity].add(value["bctx_actor_id"])
            requested.add((*identity, value["bctx_association_revision"], value["bctx_association_fact_sequence"]))
        if raw["record_kind"] == 11:
            wanted_actors[tuple(value[name] for name in contribution.BATTLE)].add(value["bc_actor_id"])
            for boundary in ("first", "last"):
                requested.add(tuple(value[name] for name in contribution.BATTLE) +
                    (value["bc_" + boundary + "_association_revision"], value["bc_" + boundary + "_association_fact_sequence"]))
    _require(len(requested) <= MAX_REFERENCE_SNAPSHOTS, "history_reference_capacity")
    grouped: dict[tuple[int, int, int, int], list[dict[str, int]]] = defaultdict(list)
    source_by_battle: dict[tuple[int, int, int], list[dict[str, int]]] = defaultdict(list)
    for row in facts.values():
        grouped[(*battle.battle_id(row), row["battle_revision"])].append(row)
        source_by_battle[battle.battle_id(row)].append(row)
    _require(len(grouped) <= MAX_PACKETS, "history_packet_capacity")
    states: dict[tuple[int, int, int], _State] = {}
    aliases: dict[tuple[int, int, int], tuple[int, int, int]] = {}
    alias_cuts: dict[tuple[int, int, int], int] = {}
    alias_receipts = {}
    bases: dict[tuple[int, int, int, int, int], tuple[dict[str, int], dict[int, _Actor], bool]] = {}
    traces: dict[tuple[int, int, int, int], list[tuple[int, int, tuple[int, ...] | None, bool]]] = defaultdict(list)
    point_boundaries = defaultdict(list)
    prefix_ranges, basis_prefixes = {}, {}
    packet_count = incomplete_count = 0
    for identity, packet_rows in sorted(grouped.items(), key=lambda item: (
            *item[0][:2], min(row["record_seq"] for row in item[1]))):
        budget.deadline()
        battle_id = identity[:3]
        if battle_id not in states:
            _require(len(states) < MAX_BATTLES, "history_battle_capacity")
            budget.reserve(OUTPUT_BYTE_BOUND)
            states[battle_id] = _State(battle_id)
        state = states[battle_id]
        _require(not state.retired and not state.close_seen, "history_fact_after_terminal_or_alias")
        ordered = sorted(packet_rows, key=lambda row: row["battle_fact_index"])
        _require(all(a["record_seq"] < b["record_seq"] for a, b in zip(ordered, ordered[1:])),
                 "history_packet_transport_order")
        first, last = ordered[0], ordered[-1]
        if incident_schema_version >= 5:
            if battle_id not in prefix_ranges:
                budget.reserve(512)
                prefix_ranges[battle_id] = (first["occurrence_utc_usec"], first["record_seq"])
            utc_first, receipt_first = prefix_ranges[battle_id]
            label = first["occurrence_utc_usec"]
            prefix_ranges[battle_id] = (None if utc_first is None or utc_first == contribution.UTC_UNKNOWN or
                label == contribution.UTC_UNKNOWN else min(utc_first, label), receipt_first)
        buffer = battle.PacketBuffer()
        for row in ordered:
            status = buffer.receive({name: row[name] for name in battle.FIELDS})
            _require(status not in ("invalid", "duplicate_conflict", "other_packet"), "history_packet_conflict")
        complete = buffer.complete
        state.packets += 1
        state.source_facts += len(ordered)
        packet_count += int(complete)
        state.incomplete_packets += int(not complete)
        incomplete_count += int(not complete)
        beginning = first["battle_fact_sequence"] - first["battle_fact_index"]
        contiguous = state.revision + 1 == first["battle_revision"] and state.last_sequence + 1 == beginning
        state.integrity &= complete and contiguous
        if not state.integrity:
            state.quality |= ROLLUP_QUALITY_PROCESS_GAP
            state.replay_verified = False
        _require(first["battle_revision"] > state.revision, "history_revision_order")
        if state.last is not None:
            _require(first["battle_start_monotonic_usec"] == state.last["battle_start_monotonic_usec"] and
                     first["battle_at_monotonic_usec"] >= state.last["battle_at_monotonic_usec"], "history_packet_clock_or_start")
            _require(state.last["battle_quality_flags"] & ~last["battle_quality_flags"] == 0,
                     "history_packet_quality_regression")
        state.revision = first["battle_revision"]
        state.last_sequence = beginning + first["battle_fact_count"] - 1
        state.quality |= last["battle_quality_flags"]
        if last["battle_quality_flags"] & battle.INCOMPLETE_GRAPH:
            state.replay_verified = False
        state.start_seen |= complete and first["battle_fact_kind"] == 1
        state.replay_verified &= state.start_seen
        terminal = complete and last["battle_fact_kind"] == 7
        at = last["battle_observed_through_monotonic_usec"] if terminal else first["battle_at_monotonic_usec"]
        _seal(state, last, budget, terminal=terminal)
        if complete and first["battle_fact_kind"] == 5:
            donor_id = tuple(first[name] for name in battle.RELATED_BATTLE)
            _require(donor_id not in aliases, "history_alias_conflict")
            donor = states.get(donor_id)
            if donor is None:
                state.integrity = state.replay_verified = False
                state.quality |= ROLLUP_QUALITY_PROCESS_GAP
            else:
                _require(not donor.close_seen and not donor.retired, "history_alias_after_terminal")
                bridge = next(row for row in ordered if row["battle_fact_kind"] == 3)
                source, target = bridge["battle_actor_id"], bridge["battle_related_actor_id"]
                connects = ((source in state.actors and target in donor.actors) or
                            (target in state.actors and source in donor.actors))
                if state.replay_verified and donor.replay_verified:
                    _require(connects, "history_alias_bridge_conflict")
                elif not connects:
                    state.integrity = state.replay_verified = False
                    state.quality |= ROLLUP_QUALITY_PROCESS_GAP
                _seal(donor, last, budget)
                _require(not state.actors.keys() & donor.actors.keys(), "history_alias_duplicate_actor")
                _require(len(state.actors) + len(donor.actors) <= battle.MAX_ACTORS, "history_alias_actor_capacity")
                budget.reserve(len(donor.actors) * ACTOR_BYTE_BOUND)
                state.actors.update({actor_id: actor.clone() for actor_id, actor in donor.actors.items()})
                for relation in state.edges:
                    state.edges[relation].update(donor.edges[relation])
                state.integrity &= donor.integrity
                state.replay_verified &= donor.replay_verified
                state.quality |= donor.quality
                if incident_schema_version >= 5:
                    own, other = prefix_ranges[battle_id], prefix_ranges[donor_id]
                    prefix_ranges[battle_id] = (None if own[0] is None or other[0] is None else min(own[0], other[0]),
                        min(own[1], other[1]))
                donor.retired = True
            aliases[donor_id] = battle_id
            alias_cuts[donor_id] = at
            if incident_schema_version >= 5:
                budget.reserve(512)
                alias_receipts[donor_id] = last["record_seq"]
        trusted = state.replay_verified and complete
        for row in ordered:
            kind = row["battle_fact_kind"]
            if kind in (2, 6):
                _accept_actor(state, row, trusted, budget)
            elif kind == 3:
                source, target = row["battle_actor_id"], row["battle_related_actor_id"]
                if source not in state.actors or target not in state.actors:
                    state.replay_verified = trusted = False
                    state.quality |= ROLLUP_QUALITY_CONTEXT_UNAVAILABLE
                _accept_actor(state, row, trusted, budget)
                if target in state.actors:
                    _require(state.actors[target].context["battle_actor_kind"] == row["battle_related_actor_kind"], "history_relation_actor_kind")
                state.edges[row["battle_relation"]].add(tuple(sorted((source, target))))
        if complete:
            active = [actor for actor in state.actors.values() if actor.active]
            owners = {actor.context["battle_actor_owner_subject_id"] for actor in active if actor.context["battle_actor_owner_subject_id"]}
            if state.replay_verified:
                _require(len(state.actors) == last["battle_actor_count"] and len(active) == last["battle_active_actor_count"] and
                         len(owners) == last["battle_observed_owner_count"], "history_roster_count_conflict")
            side_status, mode = _classify(state, last["battle_quality_flags"])
            if state.replay_verified:
                _require(side_status == last["battle_side_status"] and mode == last["battle_mode"], "history_observed_graph_conflict")
            if not state.replay_verified:
                # Partial history cannot establish otherwise absent side identities.
                for actor in state.actors.values():
                    actor.side = 0
            if terminal:
                state.close_seen = True
        state.last = last
        if incident_schema_version >= 5:
            budget.reserve(512)
            point_boundaries[battle_id].append((last["battle_revision"], last["battle_at_monotonic_usec"],
                last["record_seq"], last["battle_at_utc_usec"]))
        for actor_id in wanted_actors[battle_id]:
            budget.reserve(ACTOR_BYTE_BOUND)
            actor = state.actors.get(actor_id)
            signature = None if actor is None or not actor.active else _signature(last, actor)
            traces[(*battle_id, actor_id)].append((last["battle_revision"], last["battle_at_monotonic_usec"],
                signature, complete and state.replay_verified))
        key = (*battle_id, last["battle_revision"], last["battle_fact_sequence"])
        if complete and key in requested:
            budget.reserve(OUTPUT_BYTE_BOUND + len(state.actors) * ACTOR_BYTE_BOUND)
            bases[key] = (dict(last), {actor_id: actor.clone() for actor_id, actor in state.actors.items()}, state.replay_verified)
            if incident_schema_version >= 5:
                basis_prefixes[key] = prefix_ranges[battle_id]

    # Alias components retain separate source coverage but contribute to one
    # canonical total. Absolute inherited effort is replaced, never added twice.
    families: dict[tuple[int, int, int], list[_State]] = defaultdict(list)
    for identity, state in states.items():
        families[_canonical(identity, aliases)].append(state)
    linked = unlinked = 0
    contribution_rows = []
    totals: dict[tuple[int, int, int], dict[str, int]] = defaultdict(lambda: dict.fromkeys(COUNTERS, 0))
    available: dict[tuple[int, int, int], int] = defaultdict(int)
    contribution_counts: dict[tuple[int, int, int], int] = defaultdict(int)
    contribution_quality: dict[tuple[int, int, int], int] = defaultdict(int)
    intervals: dict[tuple[int, int, int], list[tuple[int, int]]] = defaultdict(list)
    for segment in sorted(segments.values(), key=lambda row: (*contribution.segment_key(row)[:2], row["bc_segment_seq"])):
        budget.output()
        identity = tuple(segment[name] for name in contribution.BATTLE)
        root = _canonical(identity, aliases)
        first_key = (*identity, segment["bc_first_association_revision"], segment["bc_first_association_fact_sequence"])
        last_key = (*identity, segment["bc_last_association_revision"], segment["bc_last_association_fact_sequence"])
        first_basis, last_basis = bases.get(first_key), bases.get(last_key)
        for basis, clock in ((first_basis, segment["bc_start_monotonic_usec"]),
                             (last_basis, segment["bc_observed_through_monotonic_usec"])):
            if basis is not None:
                cut = basis[0]
                _require(cut["battle_fact_kind"] == 4 and cut["battle_at_monotonic_usec"] <= clock and
                         cut["record_seq"] < segment["record_seq"], "history_contribution_reference_clock")
                _require(all(cut[name] == segment["bc_" + name.removeprefix("battle_")] for name in SCOPE),
                         "history_contribution_scope_conflict")
        if identity in alias_cuts:
            _require(segment["bc_observed_through_monotonic_usec"] <= alias_cuts[identity],
                     "history_contribution_after_alias")
        status = "verified"
        if first_basis is None or last_basis is None:
            status = "missing_packet"
        elif not first_basis[2] or not last_basis[2]:
            status = "partial_history"
        else:
            for basis in (first_basis, last_basis):
                cut, actors, _verified = basis
                actor = actors.get(segment["bc_actor_id"])
                _require(actor is not None and actor.active, "history_contribution_actor_absent")
                _require(all(actor.context[name] == segment["bc_" + name.removeprefix("battle_")] for name in ACTOR_CONTEXT) and
                         actor.side == segment["bc_actor_side"] and cut["battle_mode"] == segment["bc_mode"] and
                         cut["battle_side_status"] == segment["bc_side_status"] and
                         cut["battle_quality_flags"] == segment["bc_context_quality_flags"], "history_contribution_context_conflict")
            expected = _signature(first_basis[0], first_basis[1][segment["bc_actor_id"]])
            for revision, at, signature, verified in traces[(*identity, segment["bc_actor_id"])]:
                if revision < segment["bc_first_association_revision"] or at > segment["bc_observed_through_monotonic_usec"]:
                    continue
                if not verified:
                    status = "partial_history"
                    continue
                # A changed context beyond the last reference may seal this
                # segment exactly at its measured end, never inside the prefix.
                if at == segment["bc_observed_through_monotonic_usec"] and revision > segment["bc_last_association_revision"]:
                    continue
                _require(signature == expected, "history_contribution_crosses_context_boundary")
        terminal = states.get(identity)
        if status == "verified" and segment["bc_end_reason"] in (1, 2):
            boundary = [signature for revision, at, signature, verified in traces[(*identity, segment["bc_actor_id"])]
                if verified and at == segment["bc_observed_through_monotonic_usec"] and
                revision > segment["bc_last_association_revision"]]
            expected = _signature(first_basis[0], first_basis[1][segment["bc_actor_id"]])
            observed = (None in boundary if segment["bc_end_reason"] == 2 else
                any(signature != expected for signature in boundary) or
                alias_cuts.get(identity) == segment["bc_observed_through_monotonic_usec"])
            if not observed:
                status = "missing_lifecycle"
        if segment["bc_end_reason"] == 3:
            if terminal is None or not terminal.close_seen:
                status = "missing_lifecycle"
            else:
                close = terminal.last
                _require(all(close["battle_" + name] == segment["bc_" + name] for name in (
                    "observed_through_monotonic_usec", "observed_through_utc_usec")) and
                    close["battle_at_monotonic_usec"] == segment["bc_decision_monotonic_usec"] and
                    close["battle_at_utc_usec"] == segment["bc_decision_utc_usec"], "history_contribution_close_boundary")
                if not terminal.replay_verified:
                    status = "partial_history"
        linked += int(status == "verified")
        if status != "verified":
            unlinked += 1
        quality = segment["bc_quality_flags"]
        if status != "verified":
            quality |= ROLLUP_QUALITY_PROCESS_GAP
        first_utc, last_utc = segment["bc_start_utc_usec"], segment["bc_decision_utc_usec"]
        quality |= _utc_quality((first_utc, segment["bc_observed_through_utc_usec"], last_utc), quality)
        comparable = (first_utc != contribution.UTC_UNKNOWN and last_utc != contribution.UTC_UNKNOWN and
            first_utc <= last_utc and not quality & (battle.QUALITY_CLOCK_DISCONTINUITY |
                ROLLUP_QUALITY_UTC_UNKNOWN | ROLLUP_QUALITY_UTC_BACKWARD))
        quality |= _coverage_quality(incident_coverage, identity[:2], 11,
            first_utc if comparable else None, last_utc if comparable else None, segment["record_seq"], segment["record_seq"],
            schema_version=incident_schema_version)
        stream = (*identity[:2], segment["bc_actor_id"])
        intervals[stream].append((segment["bc_start_monotonic_usec"], segment["bc_observed_through_monotonic_usec"]))
        contribution_counts[root] += 1
        contribution_quality[root] |= quality
        available[root] |= segment["bc_available_metrics"]
        for name in COUNTERS:
            _require(segment[name] <= battle.UINT64_MAX - totals[root][name], "history_contribution_total_capacity")
            totals[root][name] += segment[name]
        contribution_rows.append({**segment, "canonical_battle": root, "link_status": status,
            "publication_quality_flags": quality, "complete_metric_coverage_implied": False})
    for spans in intervals.values():
        end = None
        for first, last in sorted(spans):
            _require(end is None or first >= end, "history_contribution_overlap")
            end = last

    build_rows = []
    for point in sorted(build_points.values(), key=builds.observation_key):
        budget.output()
        identity = tuple(point[name] for name in builds.BATTLE)
        basis = bases.get((*identity, point["bctx_association_revision"], point["bctx_association_fact_sequence"]))
        status, clock_status = "missing_packet", "unverified"
        quality = point["bctx_quality_flags"]
        at, utc = point["bctx_at_monotonic_usec"], point["bctx_at_utc_usec"]
        if basis is not None:
            cut, actors, verified = basis
            quality |= cut["battle_quality_flags"]
            _require(cut["battle_fact_kind"] == 4 and cut["battle_at_monotonic_usec"] <= at and
                cut["record_seq"] < point["record_seq"], "history_build_reference_clock")
            _require(not point["bctx_config_id"] or cut["battle_config_id"] == point["bctx_config_id"],
                "history_build_configuration_conflict")
            actor = actors.get(point["bctx_actor_id"])
            if verified:
                _require(actor is not None and actor.active and actor.context["battle_actor_kind"] == point["bctx_actor_kind"],
                    "history_build_actor_conflict")
            status = "verified" if verified else "partial_history"
            if any(revision > point["bctx_association_revision"] and boundary_at <= at and receipt < point["record_seq"]
                    for revision, boundary_at, receipt, _utc in point_boundaries[identity]):
                status = "stale_association"
            first_utc = cut["battle_at_utc_usec"]
            if utc == contribution.UTC_UNKNOWN or first_utc == contribution.UTC_UNKNOWN:
                clock_status = "unknown"
                quality |= ROLLUP_QUALITY_UTC_UNKNOWN
            elif utc - first_utc != at - cut["battle_at_monotonic_usec"]:
                clock_status = "mismatch"
                quality |= ROLLUP_QUALITY_UTC_MISMATCH
            elif quality & battle.QUALITY_CLOCK_DISCONTINUITY:
                clock_status = "discontinuous"
            else:
                clock_status = "verified"
        terminal = states.get(identity)
        if identity in alias_cuts and (at > alias_cuts[identity] or point["record_seq"] > alias_receipts[identity]) or terminal is not None and terminal.close_seen and (
                at > terminal.last["battle_observed_through_monotonic_usec"] or
                point["record_seq"] > terminal.last["record_seq"]):
            status = "outside_observed_prefix"
        if status != "verified":
            quality |= ROLLUP_QUALITY_PROCESS_GAP
        comparable = clock_status == "verified"
        quality |= _coverage_quality(incident_coverage, identity[:2], 12, utc if comparable else None,
            utc if comparable else None, point["record_seq"], point["record_seq"], schema_version=incident_schema_version)
        if basis is not None:
            # Missing association changes can invalidate a seemingly current
            # reference even when this build point itself was delivered. Review
            # its observed prefix and the interval from that prefix to the point.
            prefix_first, prefix_receipt = basis_prefixes[(*identity, point["bctx_association_revision"],
                point["bctx_association_fact_sequence"])]
            prefix_clock = comparable and prefix_first is not None
            quality |= _coverage_quality(incident_coverage, identity[:2], 10,
                prefix_first if prefix_clock else None, utc if prefix_clock else None,
                prefix_receipt, point["record_seq"], schema_version=incident_schema_version)
        build_rows.append(dict(point, canonical_battle=_canonical(identity, aliases), link_status=status,
            point_clock_status=clock_status, publication_quality_flags=quality, continuous_build_exposure_implied=False))

    control_rows, control_spans = [], defaultdict(list)
    for key, point in sorted(control_points.items()):
        budget.output()
        quality = point["ctl_quality_flags"]
        first, last, decision = (point["ctl_" + name + "_usec"] for name in ("start", "at", "decision"))
        utc_first, utc_last, utc_decision = (point["ctl_" + name + "_utc_usec"] for name in ("start", "at", "decision"))
        quality |= _utc_quality((utc_first, utc_last, utc_decision), quality)
        clock = "unknown" if controls.UTC_UNKNOWN in (utc_first, utc_last) else (
            "discontinuous" if quality & battle.QUALITY_CLOCK_DISCONTINUITY else
            "mismatch" if utc_last - utc_first != last - first else "verified")
        if clock == "mismatch":
            quality |= ROLLUP_QUALITY_UTC_MISMATCH
        links, roots = {}, {}
        for actor_prefix in ("source", "target"):
            ref = controls._association(point, actor_prefix)
            if not any(ref):
                links[actor_prefix], roots[actor_prefix] = "outside_battle", None
                continue
            identity = (*key[:2], ref[0])
            roots[actor_prefix] = _canonical(identity, aliases)
            initial_key = (*identity, *ref[1:])
            final_key = (*identity, point["ctl_last_target_association_revision"],
                point["ctl_last_target_association_fact_sequence"]) if actor_prefix == "target" else initial_key
            initial, final = bases.get(initial_key), bases.get(final_key)
            status = "missing_packet" if initial is None or final is None else "verified"
            for basis, at, utc in ((initial, first if actor_prefix == "target" else decision,
                                   utc_first if actor_prefix == "target" else utc_decision),
                                  (final, last, utc_last)):
                if basis is None:
                    continue
                cut, actors, verified = basis
                _require(cut["battle_fact_kind"] == 4 and cut["battle_at_monotonic_usec"] <= at and
                    cut["record_seq"] < point["record_seq"], "history_control_reference_clock")
                quality |= cut["battle_quality_flags"]
                if not verified:
                    status = "partial_history"
                    continue
                actor = actors.get(point["ctl_" + actor_prefix + "_actor_id"])
                _require(actor is not None and actor.active, "history_control_actor_absent")
                _require(all(actor.context[name] == item for name, item in controls.actor_context(point, actor_prefix).items()),
                    "history_control_actor_context_conflict")
                _require(not point["ctl_config_id"] or all(cut["battle_" + name] == point["ctl_" + name]
                    for name in ("config_id", "classifier_version", "policy_version")), "history_control_configuration_conflict")
                cut_utc = cut["battle_at_utc_usec"]
                if utc == controls.UTC_UNKNOWN or cut_utc == controls.UTC_UNKNOWN:
                    clock = "unknown"
                elif utc - cut_utc != at - cut["battle_at_monotonic_usec"]:
                    clock = "mismatch"
                    quality |= ROLLUP_QUALITY_UTC_MISMATCH
            if initial is not None and status == "verified":
                expected = _signature(initial[0], initial[1][point["ctl_" + actor_prefix + "_actor_id"]])
                for revision, at, signature, verified in traces[(*identity, point["ctl_" + actor_prefix + "_actor_id"])]:
                    if revision < ref[1] or at > last or at == last and revision > final_key[3]:
                        continue
                    if not verified:
                        status = "partial_history"
                    elif signature != expected:
                        status = "context_changed"
                if any(revision > final_key[3] and at < last and receipt < point["record_seq"]
                        for revision, at, receipt, _utc in point_boundaries[identity]):
                    status = "stale_association"
            terminal = states.get(identity)
            if actor_prefix == "target" and point["ctl_kind"] == 3 and point["ctl_boundary"] == 5:
                departures = {revision for revision, at, signature, verified in
                    traces[(*identity, point["ctl_target_actor_id"])] if
                    verified and signature is None and at == decision and revision > final_key[3]}
                if not departures:
                    status = "missing_lifecycle"
                elif not any(revision in departures and at == decision and utc == utc_decision
                        for revision, at, _receipt, utc in point_boundaries[identity]):
                    status = "lifecycle_mismatch"
            if point["ctl_kind"] == 3 and point["ctl_boundary"] == 6:
                if terminal is None or not terminal.close_seen:
                    status = "missing_lifecycle"
                elif terminal.last["battle_at_monotonic_usec"] != decision or terminal.last["battle_at_utc_usec"] != utc_decision:
                    status = "lifecycle_mismatch"
            if identity in alias_cuts and last > alias_cuts[identity] or terminal is not None and terminal.close_seen and (
                    last > terminal.last["battle_observed_through_monotonic_usec"]):
                status = "outside_observed_prefix"
            links[actor_prefix] = status
            if status != "verified":
                quality |= ROLLUP_QUALITY_PROCESS_GAP
            if initial_key in basis_prefixes:
                prefix_first, prefix_receipt = basis_prefixes[initial_key]
                comparable = clock == "verified" and prefix_first is not None
                quality |= _coverage_quality(incident_coverage, key[:2], 10,
                    prefix_first if comparable else None, utc_last if comparable else None,
                    prefix_receipt, point["record_seq"], schema_version=incident_schema_version)
        chain = "not_applicable"
        if point["ctl_kind"] == 2:
            chain = "entry"
        elif point["ctl_kind"] == 3:
            prior = control_points.get((*key[:2], point["ctl_previous_state_sequence"]))
            chain = "missing_predecessor" if prior is None else "verified"
            if prior is not None:
                context_fields = [name for name in controls.FIELDS if name.startswith("ctl_target_") and
                    name not in ("ctl_target_association_revision", "ctl_target_association_fact_sequence")]
                context_fields += ["ctl_" + name for name in ("environment_id", "season_id", "config_id",
                    "classifier_version", "policy_version", "build_version", "content_version", "state_available", "duration_coverage")]
                _require(prior["ctl_kind"] == 2 or prior["ctl_kind"] == 3 and prior["ctl_boundary"] == 2,
                    "history_control_predecessor_kind")
                _require(prior["record_seq"] < point["record_seq"] and prior["ctl_at_usec"] == first and
                    prior["ctl_at_utc_usec"] == utc_first and prior["ctl_after_mask"] == point["ctl_before_mask"] and
                    all(prior[name] == point[name] for name in context_fields), "history_control_chain_conflict")
            if chain != "verified":
                quality |= ROLLUP_QUALITY_PROCESS_GAP
            control_spans[(*key[:2], point["ctl_target_actor_id"], point["ctl_target_actor_kind"])].append((first, last))
        comparable = clock == "verified"
        quality |= _coverage_quality(incident_coverage, key[:2], 13,
            utc_first if comparable else None, utc_decision if comparable else None,
            point["record_seq"], point["record_seq"], schema_version=incident_schema_version)
        control_rows.append(dict(point, source_canonical_battle=roots["source"], target_canonical_battle=roots["target"],
            source_link_status=links["source"], target_link_status=links["target"], chain_status=chain,
            clock_status=clock, publication_quality_flags=quality,
            observed_prefix_usec=last - first if point["ctl_kind"] == 3 else None))
    for spans in control_spans.values():
        end = None
        for first, last in sorted(spans):
            check_deadline()
            _require(end is None or first >= end, "history_control_target_overlap")
            end = last

    battle_rows, actor_rows, exposure_rows = [], [], []
    for identity, state in sorted(states.items()):
        budget.output()
        root = _canonical(identity, aliases)
        last = state.last
        _require(last is not None, "history_missing_last_fact")
        source = source_by_battle[identity]
        occurrences = [row["occurrence_utc_usec"] for row in sorted(source, key=lambda row: row["record_seq"])]
        quality = state.quality | _utc_quality(occurrences, state.quality)
        comparable = not quality & (battle.QUALITY_CLOCK_DISCONTINUITY | ROLLUP_QUALITY_UTC_UNKNOWN | ROLLUP_QUALITY_UTC_BACKWARD)
        quality |= _coverage_quality(incident_coverage, identity[:2], 10,
            min(occurrences) if comparable else None, max(occurrences) if comparable else None,
            min(row["record_seq"] for row in source), max(row["record_seq"] for row in source), schema_version=incident_schema_version)
        for exposure in state.exposures:
            check_deadline()
            flags = exposure["quality_flags"]
            comparable = not flags & (battle.QUALITY_CLOCK_DISCONTINUITY | ROLLUP_QUALITY_UTC_UNKNOWN |
                ROLLUP_QUALITY_UTC_BACKWARD | ROLLUP_QUALITY_UTC_MISMATCH)
            flags |= _coverage_quality(incident_coverage, identity[:2], 10,
                exposure["start_utc_usec"] if comparable else None,
                exposure["observed_through_utc_usec"] if comparable else None,
                exposure["start_record_seq"], exposure["through_record_seq"], schema_version=incident_schema_version)
            exposure_rows.append(dict(exposure, canonical_battle=root, quality_flags=flags))
        canonical = identity == root
        metric_totals = {}
        for bit, names in contribution.COUNTER_FAMILIES:
            for name in names:
                metric_totals[name] = totals[root]["bc_" + name] if canonical and available[root] & bit else None
        battle_rows.append({"battle": identity, "canonical_battle": root, "canonical": canonical,
            "start_seen": state.start_seen, "close_seen": state.close_seen, "retired_by_alias": state.retired,
            "packet_history_complete": state.integrity, "observed_graph_verified": state.replay_verified,
            "source_fact_count": state.source_facts, "packet_count": state.packets,
            "incomplete_packet_count": state.incomplete_packets, "last_revision": state.revision,
            "last_fact_sequence": state.last_sequence, "declared_actor_count": last["battle_actor_count"],
            "observed_actor_count": len(state.actors), "close_reason": last["battle_close_reason"] if state.close_seen else None,
            "end_censored": bool(last["battle_end_censored"]) if state.close_seen else None,
            "observed_through_monotonic_usec": last["battle_observed_through_monotonic_usec"],
            "decision_monotonic_usec": last["battle_at_monotonic_usec"],
            "contribution_count": contribution_counts[root] if canonical else 0,
            "available_metric_mask": available[root] if canonical else 0, **metric_totals,
            "quality_flags": quality | (contribution_quality[root] if canonical else 0),
            "outcome": None, "complete_metric_coverage_implied": False,
            "account_or_controller_identity_implied": False})
        if canonical:
            for actor_id, actor in sorted(state.actors.items()):
                budget.output()
                actor_rows.append({"canonical_battle": root, **actor.context, **actor.effort,
                    "active_at_last_observation": bool(actor.active), "roles": actor.roles,
                    "effort_replay_verified": state.replay_verified, "quality_flags": quality})
    # Segments without any retained association still keep their measured values.
    # Their source identity is not upgraded into an invented battle lifecycle.
    for root in sorted(contribution_counts.keys() - families.keys()):
        budget.output()
        metrics = {name: totals[root]["bc_" + name] if available[root] & bit else None
            for bit, names in contribution.COUNTER_FAMILIES for name in names}
        battle_rows.append({"battle": root, "canonical_battle": root, "canonical": True,
            "start_seen": False, "close_seen": False, "retired_by_alias": False,
            "packet_history_complete": False, "observed_graph_verified": False,
            "source_fact_count": 0, "packet_count": 0, "incomplete_packet_count": 0,
            "last_revision": None, "last_fact_sequence": None, "declared_actor_count": None,
            "observed_actor_count": 0, "close_reason": None, "end_censored": None,
            "observed_through_monotonic_usec": None, "decision_monotonic_usec": None,
            "contribution_count": contribution_counts[root], "available_metric_mask": available[root], **metrics,
            "quality_flags": contribution_quality[root] | ROLLUP_QUALITY_PROCESS_GAP,
            "outcome": None, "complete_metric_coverage_implied": False,
            "account_or_controller_identity_implied": False})
    exposure_spans: dict[tuple[int, int, int], list[tuple[int, int]]] = defaultdict(list)
    for row in exposure_rows:
        stream = (*row["source_battle"][:2], row["battle_actor_id"])
        exposure_spans[stream].append((row["start_monotonic_usec"], row["observed_through_monotonic_usec"]))
    for spans in exposure_spans.values():
        end = None
        for first, last in sorted(spans):
            check_deadline()
            _require(end is None or first >= end, "history_actor_exposure_overlap")
            end = last
    summary = {"input_count": len(rows), "source_fact_count": len(facts), "contribution_count": len(segments),
        "identical_receipt_retries": retries, "complete_packet_count": packet_count,
        "incomplete_packet_count": incomplete_count, "alias_count": len(aliases),
        "canonical_battle_count": len({row["canonical_battle"] for row in battle_rows}),
        "verified_contribution_links": linked, "partial_contribution_links": unlinked,
        "exposure_count": len(exposure_rows),
        "verified_exposure_present_usec": sum(row["battle_present_usec"] for row in exposure_rows),
        "reserved_bytes": budget.used, "output_rows": budget.outputs,
        "atomic_publication_implied": False, "complete_metric_coverage_implied": False,
        "decisive_outcomes_available": False, "account_or_controller_identity_implied": False,
        "zero_activity_implied": False}
    if incident_schema_version >= 5:
        summary.update(build_count=len(build_rows), verified_build_links=sum(row["link_status"] == "verified" for row in build_rows),
            partial_build_links=sum(row["link_status"] != "verified" for row in build_rows))
    if incident_schema_version == 6:
        summary.update(control_count=len(control_rows))
    return BattleHistory(summary, tuple(battle_rows), tuple(actor_rows), tuple(contribution_rows), tuple(exposure_rows),
        tuple(build_rows), tuple(control_rows))
