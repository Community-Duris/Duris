"""Bounded shared-battle history and contribution linkage for publication.

Individual immutable facts are not a complete history. This reducer uses the
sealed family validators, retains missing packets and lifecycles as uncertainty,
and resolves only complete observed merge packets. It performs no database I/O
and does not establish winners, human identity or complete contribution coverage.
"""
from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
from typing import Any, Callable, Mapping, Sequence

try:
    from . import battle_contract as battle, battle_contribution_contract as contribution, incident
    from .rollup_definitions import (ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_INCIDENT_GAP, ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN,
        ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_BACKWARD)
except ImportError:
    import battle_contract as battle
    import battle_contribution_contract as contribution
    import incident
    from rollup_definitions import (ROLLUP_QUALITY_PROCESS_GAP, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_INCIDENT_GAP, ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN,
        ROLLUP_QUALITY_UTC_UNKNOWN, ROLLUP_QUALITY_UTC_BACKWARD)

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


def _seal(state: _State, at: int) -> None:
    if state.cut is None:
        state.cut = at
        return
    _require(at >= state.cut, "history_observed_clock_reversal")
    elapsed = at - state.cut
    if state.replay_verified and state.last is not None:
        owners = {side: {actor.context["battle_actor_owner_subject_id"] for actor in state.actors.values()
            if actor.active and actor.side == side and actor.context["battle_actor_owner_subject_id"]} for side in (1, 2)}
        mode = state.last["battle_mode"]
        for actor in state.actors.values():
            if not actor.active:
                continue
            additions = {"battle_present_usec": elapsed,
                "battle_contributor_usec": elapsed if actor.roles & 3 else 0,
                "battle_" + ("unknown_mode", "pve", "pvp", "mixed")[mode] + "_usec": elapsed,
                "battle_unknown_side_usec": elapsed if state.last["battle_side_status"] != 1 else 0,
                "battle_outnumbered_owner_usec": elapsed if mode == 2 and actor.context["battle_actor_owner_subject_id"] and
                    actor.side in (1, 2) and len(owners[actor.side]) < len(owners[3 - actor.side]) else 0}
            for name, amount in additions.items():
                _require(amount <= battle.UINT64_MAX - actor.effort[name], "history_effort_capacity")
                actor.effort[name] += amount
    state.cut = at


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
                      first: int | None, last: int | None, first_seq: int, last_seq: int) -> int:
    if coverage is None or coverage.get("status") in ("not_published", "not_registered"):
        return ROLLUP_QUALITY_INCIDENT_INVENTORY_UNKNOWN
    _require(coverage.get("registry_schema_version") == 4 and coverage.get("status") == "reviewed_inventory",
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
                 _integer(row.get("record_kind_mask"), lower=1, upper=(1 << 12) - 2) and row["record_kind_mask"] & 1 == 0,
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
                  incident_coverage: Mapping[str, Any] | None = None, max_output_rows: int = 2_000,
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
    _require(_integer(max_output_rows, lower=1) and _integer(max_total_bytes, lower=1) and callable(check_deadline), "history_bounds")
    budget = _Budget(max_total_bytes, max_output_rows, check_deadline)
    facts: dict[tuple[int, int, int, int], dict[str, int]] = {}
    segments: dict[tuple[int, int, int], dict[str, int]] = {}
    receipts: dict[tuple[int, int, int], dict[str, int]] = {}
    requested = set()
    wanted_actors: dict[tuple[int, int, int], set[int]] = defaultdict(set)
    retries = 0
    for raw in rows:
        budget.reserve(INPUT_BYTE_BOUND)
        _require(isinstance(raw, Mapping) and raw.get("record_kind") in (10, 11), "history_source_family")
        try:
            value = battle.validate_raw_fact(raw) if raw["record_kind"] == 10 else contribution.validate_raw_segment(raw)
        except (battle.BattleContractError, contribution.ContributionContractError) as error:
            raise HistoryError(str(error)) from error
        prefix = "battle_" if raw["record_kind"] == 10 else "bc_"
        _require((value[prefix + "environment_id"], value[prefix + "season_id"]) == scope, "history_source_scope")
        retained = dict(value, **{name: raw[name] for name in HEADER})
        receipt = tuple(raw[name] for name in HEADER[:3])
        if receipt in receipts:
            _require(receipts[receipt] == retained, "history_transport_conflict")
            retries += 1
            continue
        receipts[receipt] = retained
        target, key = (facts, battle.fact_key(value)) if raw["record_kind"] == 10 else (segments, contribution.segment_key(value))
        _require(key not in target, "history_logical_receipt_conflict")
        target[key] = retained
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
    bases: dict[tuple[int, int, int, int, int], tuple[dict[str, int], dict[int, _Actor], bool]] = {}
    traces: dict[tuple[int, int, int, int], list[tuple[int, int, tuple[int, ...] | None, bool]]] = defaultdict(list)
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
        _seal(state, at)
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
                _seal(donor, at)
                _require(not state.actors.keys() & donor.actors.keys(), "history_alias_duplicate_actor")
                _require(len(state.actors) + len(donor.actors) <= battle.MAX_ACTORS, "history_alias_actor_capacity")
                budget.reserve(len(donor.actors) * ACTOR_BYTE_BOUND)
                state.actors.update({actor_id: actor.clone() for actor_id, actor in donor.actors.items()})
                for relation in state.edges:
                    state.edges[relation].update(donor.edges[relation])
                state.integrity &= donor.integrity
                state.replay_verified &= donor.replay_verified
                state.quality |= donor.quality
                donor.retired = True
            aliases[donor_id] = battle_id
            alias_cuts[donor_id] = at
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
            first_utc if comparable else None, last_utc if comparable else None, segment["record_seq"], segment["record_seq"])
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

    battle_rows, actor_rows = [], []
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
            min(row["record_seq"] for row in source), max(row["record_seq"] for row in source))
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
    summary = {"input_count": len(rows), "source_fact_count": len(facts), "contribution_count": len(segments),
        "identical_receipt_retries": retries, "complete_packet_count": packet_count,
        "incomplete_packet_count": incomplete_count, "alias_count": len(aliases),
        "canonical_battle_count": len({row["canonical_battle"] for row in battle_rows}),
        "verified_contribution_links": linked, "partial_contribution_links": unlinked,
        "reserved_bytes": budget.used, "output_rows": budget.outputs,
        "atomic_publication_implied": False, "complete_metric_coverage_implied": False,
        "decisive_outcomes_available": False, "account_or_controller_identity_implied": False,
        "zero_activity_implied": False}
    return BattleHistory(summary, tuple(battle_rows), tuple(actor_rows), tuple(contribution_rows))
