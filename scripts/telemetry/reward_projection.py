"""Bounded, read-only source adapters for committed reward projection.

The worker in this module is intentionally separate from the generic rollup
engine.  It reads committed ledgers and outcome context with explicit column
lists, projects one source identity at a time, and writes only the projection
tables introduced by migration 0023.  It never writes gameplay tables, uses
the critical outbox delivery state, or locks a gameplay row.

The CLI is an external worker entry point.  It uses the dedicated
``TELEMETRY_REWARD_DB_*`` environment namespace and is not imported by the
game server.
"""
from __future__ import annotations

from argparse import ArgumentParser
from dataclasses import dataclass, replace
from datetime import datetime, timezone
import json
import sys
from typing import Any, Callable, Mapping, Protocol, Sequence

try:  # Running as a package.
    from .reward_projection_definitions import (
        AuthorityKind,
        MAX_REPORT_WINDOW_USEC,
        MAX_PAGE_SIZE,
        MAX_QUERY_STATEMENTS_PER_PAGE,
        ProjectionBatch,
        ProjectionState,
        ProjectionStatus,
        RewardKind,
        REWARD_REPORT_DEFINITION,
        SourceCursor,
        SourceKind,
        SourceObservation,
        ZERO_OPERATION_ID,
        normalize_operation_id,
        project_observations,
    )
except ImportError:  # Running scripts/telemetry/reward_projection.py directly.
    from reward_projection_definitions import (  # type: ignore[no-redef]
        AuthorityKind,
        MAX_REPORT_WINDOW_USEC,
        MAX_PAGE_SIZE,
        MAX_QUERY_STATEMENTS_PER_PAGE,
        ProjectionBatch,
        ProjectionState,
        ProjectionStatus,
        RewardKind,
        REWARD_REPORT_DEFINITION,
        SourceCursor,
        SourceKind,
        SourceObservation,
        ZERO_OPERATION_ID,
        normalize_operation_id,
        project_observations,
    )


class RewardProjectionError(RuntimeError):
    """Base error for the bounded projection worker."""


class ProjectionBoundsExceeded(RewardProjectionError):
    """A page or worker statement budget was exceeded."""


class AmbiguousCommit(RewardProjectionError):
    """The driver did not acknowledge COMMIT; state must be reread before retry."""


class UnsupportedSourceError(RewardProjectionError):
    """A legacy source has no safe committed identity."""


class ConnectionFactory(Protocol):
    def connect(self) -> Any: ...

    def close(self, connection: Any | None = None) -> None: ...


RowFactory = Callable[[Mapping[str, Any]], SourceObservation]


def _timestamp_usec(value: Any) -> int:
    if isinstance(value, datetime):
        timestamp = value if value.tzinfo is not None else value.replace(tzinfo=timezone.utc)
        return int(timestamp.timestamp() * 1_000_000)
    if isinstance(value, str):
        normalized = value.replace("Z", "+00:00")
        parsed = datetime.fromisoformat(normalized)
        timestamp = parsed if parsed.tzinfo is not None else parsed.replace(tzinfo=timezone.utc)
        return int(timestamp.timestamp() * 1_000_000)
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        return int(value)
    raise ValueError("source created_at must be a datetime, ISO timestamp, or integer usec")


def _utc_now_usec() -> int:
    return int(datetime.now(timezone.utc).timestamp() * 1_000_000)


def _operation(row: Mapping[str, Any]) -> bytes:
    return normalize_operation_id(row["operation_id"])


def _integer(row: Mapping[str, Any], name: str, default: int = 0) -> int:
    value = row.get(name, default)
    if value is None:
        return default
    return int(value)


def _boolean(row: Mapping[str, Any], name: str, default: bool = False) -> bool:
    return bool(_integer(row, name, int(default)))


def _context_observation(row: Mapping[str, Any], *, source_kind: SourceKind,
                         source_table: str) -> SourceObservation:
    return SourceObservation(
        source_kind=source_kind,
        operation_id=_operation(row),
        entry_index=_integer(row, "entry_index"),
        participant_pid=_integer(row, "participant_pid"),
        source_table=source_table,
        created_at_usec=_timestamp_usec(row["created_at"]),
        authority_kind=AuthorityKind.OUTCOME_CONTEXT,
        reason_type=_integer(row, "reason_type"),
        reason_id=_integer(row, "reason_id"),
        source_site=_integer(row, "source_site"),
        ledger_operation_id=(
            normalize_operation_id(row["ledger_operation_id"])
            if row.get("ledger_operation_id") is not None else None
        ),
        is_creation=_boolean(row, "is_creation"),
        context_complete=_boolean(row, "context_complete", True),
    )


def _currency_observation(row: Mapping[str, Any]) -> SourceObservation:
    net = _integer(row, "net_amount")
    gross = _integer(row, "gross_amount")
    is_transfer = _boolean(row, "is_transfer")
    transfer = _integer(row, "transfer_amount") if is_transfer else None
    return SourceObservation(
        source_kind=SourceKind.CURRENCY_LEDGER,
        operation_id=_operation(row),
        entry_index=0,
        participant_pid=_integer(row, "participant_pid"),
        source_table="currency_ledger",
        created_at_usec=_timestamp_usec(row["created_at"]),
        authority_kind=AuthorityKind.COMMITTED_LEDGER,
        reward_kind=RewardKind.CURRENCY,
        gross_amount=gross,
        net_amount=net,
        transfer_amount=transfer,
        reason_type=_integer(row, "reason_type"),
        reason_id=_integer(row, "reason_id"),
        source_site=_integer(row, "source_site"),
        is_transfer=is_transfer,
        is_creation=_boolean(row, "is_creation"),
    )


def _epic_observation(row: Mapping[str, Any]) -> SourceObservation:
    net = _integer(row, "net_amount")
    return SourceObservation(
        source_kind=SourceKind.EPIC_LEDGER,
        operation_id=_operation(row),
        entry_index=0,
        participant_pid=_integer(row, "participant_pid"),
        source_table="epic_ledger",
        created_at_usec=_timestamp_usec(row["created_at"]),
        authority_kind=AuthorityKind.COMMITTED_LEDGER,
        reward_kind=RewardKind.EPIC,
        gross_amount=max(net, 0),
        net_amount=net,
        reason_type=_integer(row, "reason_type"),
        reason_id=_integer(row, "reason_id"),
        source_site=_integer(row, "source_site"),
        is_creation=net > 0,
    )


def _frag_observation(row: Mapping[str, Any]) -> SourceObservation:
    net = _integer(row, "net_amount")
    return SourceObservation(
        source_kind=SourceKind.COMBAT_FRAG_LEDGER,
        operation_id=_operation(row),
        entry_index=_integer(row, "entry_index"),
        participant_pid=_integer(row, "participant_pid"),
        source_table="combat_frag_ledger",
        created_at_usec=_timestamp_usec(row["created_at"]),
        authority_kind=AuthorityKind.COMMITTED_LEDGER,
        reward_kind=RewardKind.FRAGS,
        gross_amount=max(net, 0),
        net_amount=net,
        reason_type=_integer(row, "reason_type"),
        reason_id=_integer(row, "reason_id"),
        source_site=_integer(row, "source_site"),
        is_creation=net > 0,
    )


_LEDGER_LINK_SQL = (
    "(CASE WHEN EXISTS (SELECT 1 FROM currency_ledger AS cl "
    "WHERE cl.operation_id=o.operation_id) OR EXISTS (SELECT 1 FROM epic_ledger AS el "
    "WHERE el.operation_id=o.operation_id) OR EXISTS (SELECT 1 FROM combat_frag_ledger AS fl "
    "WHERE fl.operation_id=o.operation_id) THEN o.operation_id ELSE NULL END) AS ledger_operation_id"
)


def _zone_parent_observation(row: Mapping[str, Any]) -> SourceObservation:
    return _context_observation(row, source_kind=SourceKind.ZONE_OUTCOME,
                                source_table="zone_touch_outcome")


def _zone_participant_observation(row: Mapping[str, Any]) -> SourceObservation:
    return _context_observation(row, source_kind=SourceKind.ZONE_OUTCOME_PARTICIPANT,
                                source_table="zone_touch_outcome_participant")


def _pvp_parent_observation(row: Mapping[str, Any]) -> SourceObservation:
    return _context_observation(row, source_kind=SourceKind.PVP_OUTCOME,
                                source_table="combat_outcome")


def _pvp_participant_observation(row: Mapping[str, Any]) -> SourceObservation:
    return _context_observation(row, source_kind=SourceKind.PVP_OUTCOME_PARTICIPANT,
                                source_table="combat_outcome_participant")


def _boon_parent_observation(row: Mapping[str, Any]) -> SourceObservation:
    return _context_observation(row, source_kind=SourceKind.BOON_OUTCOME,
                                source_table="boon_reward_outcome")


def _boon_entry_observation(row: Mapping[str, Any]) -> SourceObservation:
    return _context_observation(row, source_kind=SourceKind.BOON_OUTCOME_ENTRY,
                                source_table="boon_reward_outcome_entry")


@dataclass(frozen=True, slots=True)
class SourceAdapter:
    """An explicit, keyset-paginated source query and row decoder."""

    name: str
    source_kind: SourceKind
    table: str
    from_sql: str
    select_sql: str | None
    row_factory: RowFactory | None
    context_only: bool
    description: str
    key_created_sql: str = "created_at"
    key_operation_sql: str = "operation_id"
    key_entry_sql: str = "entry_index"
    key_participant_sql: str = "participant_pid"

    @property
    def supported(self) -> bool:
        return self.select_sql is not None and self.row_factory is not None

    def validate(self) -> None:
        if not self.name or not self.table or not self.from_sql:
            raise ValueError("reward source adapter names and tables are required")
        if not self.supported:
            return
        if "SELECT *" in self.select_sql.upper():
            raise ValueError(f"{self.name} cannot use SELECT *")
        if "FOR UPDATE" in self.select_sql.upper() or "OFFSET" in self.select_sql.upper():
            raise ValueError(f"{self.name} cannot lock or offset source rows")


_CURRENCY_WALLET_VALUE = (
    "(l.wallet_delta_copper + 10*l.wallet_delta_silver + "
    "100*l.wallet_delta_gold + 1000*l.wallet_delta_platinum)"
)
_CURRENCY_BANK_VALUE = (
    "(l.bank_delta_copper + 10*l.bank_delta_silver + "
    "100*l.bank_delta_gold + 1000*l.bank_delta_platinum)"
)
_CURRENCY_NET_VALUE = f"({_CURRENCY_WALLET_VALUE}+{_CURRENCY_BANK_VALUE})"
_CURRENCY_TRANSFER_VALUE = (
    f"(CASE WHEN {_CURRENCY_NET_VALUE}=0 AND "
    f"({_CURRENCY_WALLET_VALUE}<>0 OR {_CURRENCY_BANK_VALUE}<>0) "
    f"THEN ABS({_CURRENCY_WALLET_VALUE}) ELSE 0 END)"
)
_CURRENCY_IS_TRANSFER = (
    f"(CASE WHEN {_CURRENCY_NET_VALUE}=0 AND "
    f"({_CURRENCY_WALLET_VALUE}<>0 OR {_CURRENCY_BANK_VALUE}<>0) THEN 1 ELSE 0 END)"
)
_CURRENCY_IS_CREATION = f"(CASE WHEN {_CURRENCY_NET_VALUE}>0 THEN 1 ELSE 0 END)"


SOURCE_ADAPTERS: tuple[SourceAdapter, ...] = (
    SourceAdapter(
        name="currency_ledger",
        source_kind=SourceKind.CURRENCY_LEDGER,
        table="currency_ledger",
        from_sql="currency_ledger AS l",
        select_sql=(
            "l.operation_id AS operation_id, 0 AS entry_index, l.pid AS participant_pid, "
            "l.created_at AS created_at, l.reason_type AS reason_type, "
            "l.reason_id AS reason_id, l.source_site AS source_site, "
            f"{_CURRENCY_NET_VALUE} AS net_amount, "
            f"GREATEST({_CURRENCY_NET_VALUE},0) AS gross_amount, "
            f"{_CURRENCY_TRANSFER_VALUE} AS transfer_amount, "
            f"{_CURRENCY_IS_TRANSFER} AS is_transfer, "
            f"{_CURRENCY_IS_CREATION} AS is_creation"
        ),
        row_factory=_currency_observation,
        context_only=False,
        description=(
            "Committed wallet and bank denomination deltas. Positive net value is "
            "creation, zero-net movement is transfer, and negative net value is sink."
        ),
    ),
    SourceAdapter(
        name="epic_ledger",
        source_kind=SourceKind.EPIC_LEDGER,
        table="epic_ledger",
        from_sql="epic_ledger AS l",
        select_sql=(
            "l.operation_id AS operation_id, 0 AS entry_index, l.pid AS participant_pid, "
            "l.created_at AS created_at, l.delta AS net_amount, l.reason_type AS reason_type, "
            "l.reason_id AS reason_id, l.source_site AS source_site"
        ),
        row_factory=_epic_observation,
        context_only=False,
        description="Committed epic delta ledger; outcomes are context only.",
    ),
    SourceAdapter(
        name="combat_frag_ledger",
        source_kind=SourceKind.COMBAT_FRAG_LEDGER,
        table="combat_frag_ledger",
        from_sql="combat_frag_ledger AS l",
        select_sql=(
            "l.operation_id AS operation_id, l.participant_index AS entry_index, "
            "l.pid AS participant_pid, l.created_at AS created_at, l.delta AS net_amount"
        ),
        row_factory=_frag_observation,
        context_only=False,
        description="Committed PvP frag delta ledger; outcome rows are context only.",
    ),
    SourceAdapter(
        name="zone_touch_outcome",
        source_kind=SourceKind.ZONE_OUTCOME,
        table="zone_touch_outcome",
        from_sql="zone_touch_outcome AS o",
        select_sql=(
            "o.operation_id AS operation_id, 0 AS entry_index, o.toucher_pid AS participant_pid, "
            "o.created_at AS created_at, " + _LEDGER_LINK_SQL + ", 1 AS context_complete, "
            "CASE WHEN o.epic_value > 0 THEN 1 ELSE 0 END AS is_creation"
        ),
        row_factory=_zone_parent_observation,
        context_only=True,
        description="Zone parent outcome context; never an authority amount.",
    ),
    SourceAdapter(
        name="zone_touch_outcome_participant",
        source_kind=SourceKind.ZONE_OUTCOME_PARTICIPANT,
        table="zone_touch_outcome_participant",
        from_sql="zone_touch_outcome_participant AS p JOIN zone_touch_outcome AS o ON o.operation_id=p.operation_id",
        select_sql=(
            "p.operation_id AS operation_id, p.participant_index AS entry_index, "
            "p.pid AS participant_pid, o.created_at AS created_at, "
            + _LEDGER_LINK_SQL + ", 1 AS context_complete, "
            "CASE WHEN p.epic_value > 0 THEN 1 ELSE 0 END AS is_creation"
        ),
        row_factory=_zone_participant_observation,
        context_only=True,
        description="Zone participant context; parent/participant rows do not add rewards.",
        key_created_sql="o.created_at",
        key_operation_sql="p.operation_id",
        key_entry_sql="p.participant_index",
        key_participant_sql="p.pid",
    ),
    SourceAdapter(
        name="combat_outcome",
        source_kind=SourceKind.PVP_OUTCOME,
        table="combat_outcome",
        from_sql="combat_outcome AS o",
        select_sql=(
            "o.operation_id AS operation_id, 0 AS entry_index, o.victim_pid AS participant_pid, "
            "o.created_at AS created_at, " + _LEDGER_LINK_SQL + ", 1 AS context_complete"
        ),
        row_factory=_pvp_parent_observation,
        context_only=True,
        description="PvP outcome parent context; committed epic/currency/frag ledgers are authority.",
    ),
    SourceAdapter(
        name="combat_outcome_participant",
        source_kind=SourceKind.PVP_OUTCOME_PARTICIPANT,
        table="combat_outcome_participant",
        from_sql="combat_outcome_participant AS p JOIN combat_outcome AS o ON o.operation_id=p.operation_id",
        select_sql=(
            "p.operation_id AS operation_id, p.participant_index AS entry_index, "
            "p.pid AS participant_pid, o.created_at AS created_at, "
            + _LEDGER_LINK_SQL + ", 1 AS context_complete"
        ),
        row_factory=_pvp_participant_observation,
        context_only=True,
        description="PvP participant context; participant deltas are not re-counted as rewards.",
        key_created_sql="o.created_at",
        key_operation_sql="p.operation_id",
        key_entry_sql="p.participant_index",
        key_participant_sql="p.pid",
    ),
    SourceAdapter(
        name="boon_reward_outcome",
        source_kind=SourceKind.BOON_OUTCOME,
        table="boon_reward_outcome",
        from_sql="boon_reward_outcome AS o",
        select_sql=(
            "o.operation_id AS operation_id, 0 AS entry_index, o.pid AS participant_pid, "
            "o.created_at AS created_at, " + _LEDGER_LINK_SQL + ", 1 AS context_complete"
        ),
        row_factory=_boon_parent_observation,
        context_only=True,
        description="Boon outcome parent context; numeric reward values need ledger linkage.",
    ),
    SourceAdapter(
        name="boon_reward_outcome_entry",
        source_kind=SourceKind.BOON_OUTCOME_ENTRY,
        table="boon_reward_outcome_entry",
        from_sql="boon_reward_outcome_entry AS e JOIN boon_reward_outcome AS o ON o.operation_id=e.operation_id",
        select_sql=(
            "e.operation_id AS operation_id, e.entry_index AS entry_index, o.pid AS participant_pid, "
            "o.created_at AS created_at, " + _LEDGER_LINK_SQL + ", 1 AS context_complete"
        ),
        row_factory=_boon_entry_observation,
        context_only=True,
        description="Boon entry context; parent and entries do not add ledger rewards.",
        key_created_sql="o.created_at",
        key_operation_sql="e.operation_id",
        key_entry_sql="e.entry_index",
        key_participant_sql="o.pid",
    ),
    SourceAdapter(
        name="legacy_world_quest_accomplished",
        source_kind=SourceKind.QUEST_OUTCOME,
        table="world_quest_accomplished",
        from_sql="world_quest_accomplished AS q",
        select_sql=None,
        row_factory=None,
        context_only=True,
        description=(
            "Not projected: the legacy table has no committed operation_id. "
            "Do not fabricate an identity; historical context remains unknown."
        ),
    ),
    SourceAdapter(
        name="legacy_quest_trophy",
        source_kind=SourceKind.QUEST_OUTCOME,
        table="quest_trophy",
        from_sql="quest_trophy AS q",
        select_sql=None,
        row_factory=None,
        context_only=True,
        description=(
            "Not projected: the legacy table has no committed operation_id. "
            "Do not fabricate an identity; historical context remains unknown."
        ),
    ),
)

for _adapter in SOURCE_ADAPTERS:
    _adapter.validate()

SOURCE_ADAPTER_BY_NAME = {adapter.name: adapter for adapter in SOURCE_ADAPTERS}
SUPPORTED_SOURCE_ADAPTERS = tuple(adapter for adapter in SOURCE_ADAPTERS if adapter.supported)


def _cursor_predicate(adapter: SourceAdapter, cursor: SourceCursor) -> tuple[str, tuple[Any, ...]]:
    if cursor.operation_id == ZERO_OPERATION_ID:
        return (
            adapter.key_created_sql + " >= %s",
            (datetime.fromtimestamp(cursor.created_at_usec / 1_000_000, timezone.utc).replace(tzinfo=None),),
        )
    predicate = (
        f"({adapter.key_created_sql} > %s OR ({adapter.key_created_sql} = %s AND "
        f"({adapter.key_operation_sql} > %s OR ({adapter.key_operation_sql} = %s AND "
        f"({adapter.key_entry_sql} > %s OR ({adapter.key_entry_sql} = %s AND "
        f"{adapter.key_participant_sql} > %s))))))"
    )
    return (
        predicate,
        (
            datetime.fromtimestamp(cursor.created_at_usec / 1_000_000, timezone.utc).replace(tzinfo=None),
            datetime.fromtimestamp(cursor.created_at_usec / 1_000_000, timezone.utc).replace(tzinfo=None),
            cursor.operation_id,
            cursor.operation_id,
            cursor.entry_index,
            cursor.entry_index,
            cursor.participant_pid,
        ),
    )


def build_source_page_query(adapter: SourceAdapter, *, cursor: SourceCursor,
                            high_water_usec: int, page_size: int) -> tuple[str, tuple[Any, ...]]:
    """Build one bounded keyset query; source SQL is never caller-provided."""

    adapter.validate()
    if not adapter.supported:
        raise UnsupportedSourceError(adapter.description)
    if isinstance(page_size, bool) or not isinstance(page_size, int) or not 1 <= page_size <= MAX_PAGE_SIZE:
        raise ValueError(f"page_size must be between 1 and {MAX_PAGE_SIZE}")
    if high_water_usec < cursor.created_at_usec:
        raise ValueError("high-water timestamp cannot precede the cursor")
    cursor.validate()
    high_water = datetime.fromtimestamp(high_water_usec / 1_000_000, timezone.utc).replace(tzinfo=None)
    predicate, predicate_parameters = _cursor_predicate(adapter, cursor)
    sql = (
        "SELECT " + adapter.select_sql + " FROM " + adapter.from_sql +
        " WHERE " + adapter.key_created_sql + " <= %s AND " + predicate +
        " ORDER BY " + adapter.key_created_sql + "," + adapter.key_operation_sql + "," +
        adapter.key_entry_sql + "," + adapter.key_participant_sql + " LIMIT %s"
    )
    return sql, (high_water,) + predicate_parameters + (page_size,)


def project_source_page(observations: Sequence[SourceObservation]) -> ProjectionBatch:
    """Pure page projection, exposed for guarded tests and offline planning."""

    return project_observations(observations)


def build_reward_report_query(*, start_usec: int, end_usec: int,
                              max_rows: int = MAX_PAGE_SIZE) -> tuple[str, tuple[Any, ...]]:
    """Build the bounded read-only aggregate query for published projection rows."""

    if start_usec <= 0 or end_usec <= start_usec or end_usec - start_usec > MAX_REPORT_WINDOW_USEC:
        raise ValueError("reward report range must be positive and at most 31 days")
    if isinstance(max_rows, bool) or not isinstance(max_rows, int) or not 1 <= max_rows <= MAX_PAGE_SIZE:
        raise ValueError(f"max_rows must be between 1 and {MAX_PAGE_SIZE}")
    query = (
        "SELECT source_kind,reward_kind,status,COUNT(*) AS source_count,"
        "SUM(CASE WHEN status=1 AND authority_kind=1 THEN COALESCE(gross_amount,0) ELSE 0 END) AS gross_amount,"
        "SUM(CASE WHEN status=1 AND authority_kind=1 THEN COALESCE(net_amount,0) ELSE 0 END) AS net_amount,"
        "SUM(CASE WHEN status=1 AND authority_kind=1 THEN COALESCE(transfer_amount,0) ELSE 0 END) AS transfer_amount,"
        "SUM(CASE WHEN status=3 THEN 1 ELSE 0 END) AS conflict_count,"
        "SUM(CASE WHEN status=5 THEN 1 ELSE 0 END) AS unknown_context_count,"
        "MIN(source_created_at) AS coverage_start,MAX(source_created_at) AS coverage_end,"
        "BIT_OR(quality_flags) AS quality_flags "
        "FROM telemetry_reward_projection "
        "WHERE source_created_at >= %s AND source_created_at < %s "
        "GROUP BY source_kind,reward_kind,status "
        "ORDER BY source_kind,reward_kind,status LIMIT %s"
    )
    return query, (
        _db_time(start_usec), _db_time(end_usec), max_rows,
    )


def build_reward_state_query() -> str:
    """Return the fixed-column read-only coverage query."""

    return (
        "SELECT source_kind,cycle_id,fast_cursor_created_at,fast_cursor_operation_id,"
        "fast_cursor_entry_index,fast_cursor_participant_pid,reconcile_cursor_created_at,"
        "reconcile_cursor_operation_id,reconcile_cursor_entry_index,reconcile_cursor_participant_pid,"
        "cycle_high_water,retention_floor,acknowledged_through,backlog_rows,quality_flags,"
        "provisional,last_fast_started_at,last_fast_completed_at,last_reconcile_started_at,"
        "last_reconcile_completed_at,updated_at FROM telemetry_reward_projection_state "
        "ORDER BY source_kind LIMIT 10"
    )


PROJECTION_INSERT_COLUMNS = (
    "source_kind", "operation_id", "entry_index", "participant_pid", "source_table",
    "source_created_at", "authority_kind", "reward_kind", "gross_amount", "net_amount",
    "transfer_amount", "parent_operation_id", "economic_operation_id", "reason_type",
    "reason_id", "source_site", "is_transfer", "is_creation", "status", "quality_flags",
    "source_payload_digest", "cycle_id", "context_complete",
)

STATE_COLUMNS = (
    "source_kind", "cycle_id", "fast_cursor_created_at", "fast_cursor_operation_id",
    "fast_cursor_entry_index", "fast_cursor_participant_pid", "reconcile_cursor_created_at",
    "reconcile_cursor_operation_id", "reconcile_cursor_entry_index",
    "reconcile_cursor_participant_pid", "cycle_high_water", "retention_floor",
    "acknowledged_through", "backlog_rows", "quality_flags", "provisional",
    "last_fast_started_at", "last_fast_completed_at", "last_reconcile_started_at",
    "last_reconcile_completed_at",
)


def _db_time(usec: int) -> datetime | None:
    if usec <= 0:
        return None
    return datetime.fromtimestamp(usec / 1_000_000, timezone.utc).replace(tzinfo=None)


def _record_parameters(record: Any, cycle_id: int) -> tuple[Any, ...]:
    return (
        int(record.source_kind), record.operation_id, record.entry_index, record.participant_pid,
        record.source_table, _db_time(record.created_at_usec), int(record.authority_kind),
        int(record.reward_kind), record.gross_amount, record.net_amount, record.transfer_amount,
        record.parent_operation_id, record.economic_operation_id, record.reason_type,
        record.reason_id, record.source_site,
        int(record.is_transfer), int(record.is_creation), int(record.status),
        int(record.quality_flags), record.payload_digest, cycle_id, int(record.context_complete),
    )


def _state_parameters(state: ProjectionState) -> tuple[Any, ...]:
    state.validate()
    return (
        int(state.source_kind), state.cycle_id, _db_time(state.fast_cursor.created_at_usec),
        state.fast_cursor.operation_id, state.fast_cursor.entry_index, state.fast_cursor.participant_pid,
        _db_time(state.reconcile_cursor.created_at_usec), state.reconcile_cursor.operation_id,
        state.reconcile_cursor.entry_index, state.reconcile_cursor.participant_pid,
        _db_time(state.cycle_high_water_usec), _db_time(state.retention_floor_usec),
        _db_time(state.acknowledged_through_usec), state.backlog_rows, state.quality_flags,
        int(state.provisional), _db_time(state.fast_started_at_usec),
        _db_time(state.fast_completed_at_usec), _db_time(state.reconcile_started_at_usec),
        _db_time(state.reconcile_completed_at_usec),
    )


class RewardProjectionStore:
    """One-connection, bounded projection sink."""

    def __init__(self, connection_factory: ConnectionFactory, *, page_size: int = 256,
                 statement_limit: int = MAX_QUERY_STATEMENTS_PER_PAGE) -> None:
        if isinstance(page_size, bool) or not isinstance(page_size, int) or not 1 <= page_size <= MAX_PAGE_SIZE:
            raise ValueError(f"page_size must be between 1 and {MAX_PAGE_SIZE}")
        if isinstance(statement_limit, bool) or not isinstance(statement_limit, int) or not 1 <= statement_limit <= MAX_QUERY_STATEMENTS_PER_PAGE:
            raise ValueError(f"statement_limit must be between 1 and {MAX_QUERY_STATEMENTS_PER_PAGE}")
        self.connection_factory = connection_factory
        self.page_size = page_size
        self.statement_limit = statement_limit
        self._connection: Any | None = None
        self._statement_count = 0

    @property
    def connection_count(self) -> int:
        return int(self._connection is not None)

    @property
    def statement_count(self) -> int:
        return self._statement_count

    def _ensure_connection(self) -> Any:
        if self._connection is None:
            self._connection = self.connection_factory.connect()
        return self._connection

    def _execute(self, statement: str, parameters: Sequence[Any] = ()) -> list[Mapping[str, Any]]:
        self._statement_count += 1
        if self._statement_count > self.statement_limit:
            raise ProjectionBoundsExceeded("reward projection statement budget exceeded")
        cursor = self._ensure_connection().cursor()
        try:
            cursor.execute(statement, tuple(parameters))
            description = getattr(cursor, "description", None)
            return list(cursor.fetchall()) if description else []
        finally:
            try:
                cursor.close()
            except Exception:
                pass

    def _begin(self) -> None:
        connection = self._ensure_connection()
        begin = getattr(connection, "begin", None)
        if begin is not None:
            begin()
        else:
            self._execute("START TRANSACTION")

    def _rollback(self) -> None:
        if self._connection is not None:
            self._connection.rollback()

    def _commit(self) -> None:
        connection = self._ensure_connection()
        try:
            connection.commit()
        except Exception as error:
            # The server may have committed before the acknowledgement was
            # lost.  Discard this socket so a caller cannot issue another
            # page without rereading the durable state.
            self._connection = None
            try:
                self.connection_factory.close(connection)
            except Exception:
                try:
                    connection.close()
                except Exception:
                    pass
            raise AmbiguousCommit("reward projection COMMIT acknowledgement is ambiguous") from error

    def close(self) -> None:
        if self._connection is None:
            return
        connection = self._connection
        self._connection = None
        try:
            self.connection_factory.close(connection)
        except Exception:
            connection.close()

    def fetch_page(self, adapter: SourceAdapter, *, cursor: SourceCursor,
                   high_water_usec: int) -> tuple[SourceObservation, ...]:
        sql, parameters = build_source_page_query(
            adapter, cursor=cursor, high_water_usec=high_water_usec, page_size=self.page_size
        )
        rows = self._execute(sql, parameters)
        if len(rows) > self.page_size:
            raise ProjectionBoundsExceeded("source adapter returned more than its LIMIT")
        if adapter.row_factory is None:
            raise UnsupportedSourceError(adapter.description)
        return tuple(adapter.row_factory(row) for row in rows)

    def upsert_projection(self, batch: ProjectionBatch, *, cycle_id: int) -> None:
        if len(batch.records) > self.page_size:
            raise ProjectionBoundsExceeded("projection batch exceeds page size")
        if not batch.records:
            return
        placeholders = "(" + ",".join(["%s"] * len(PROJECTION_INSERT_COLUMNS)) + ")"
        same_payload = (
            "telemetry_reward_projection.source_payload_digest="
            "VALUES(source_payload_digest)"
        )
        sql = (
            "INSERT INTO telemetry_reward_projection (" + ",".join(PROJECTION_INSERT_COLUMNS) + ") VALUES "
            + ",".join([placeholders] * len(batch.records))
            + " ON DUPLICATE KEY UPDATE "
            + "source_table=IF(" + same_payload + ",telemetry_reward_projection.source_table,VALUES(source_table)),"
            + "source_created_at=IF(" + same_payload + ",telemetry_reward_projection.source_created_at,VALUES(source_created_at)),"
            + "authority_kind=IF(VALUES(status)=3 OR NOT(" + same_payload + "),0,telemetry_reward_projection.authority_kind),"
            + "reward_kind=IF(VALUES(status)=3 OR NOT(" + same_payload + "),0,telemetry_reward_projection.reward_kind),"
            + "gross_amount=IF(VALUES(status)=3 OR NOT(" + same_payload + "),NULL,telemetry_reward_projection.gross_amount),"
            + "net_amount=IF(VALUES(status)=3 OR NOT(" + same_payload + "),NULL,telemetry_reward_projection.net_amount),"
            + "transfer_amount=IF(VALUES(status)=3 OR NOT(" + same_payload + "),NULL,telemetry_reward_projection.transfer_amount),"
            + "status=IF(VALUES(status)=3 OR NOT(" + same_payload + "),3,telemetry_reward_projection.status),"
            + "quality_flags=telemetry_reward_projection.quality_flags | IF(" + same_payload + ",VALUES(quality_flags),2),"
            + "source_payload_digest=VALUES(source_payload_digest),cycle_id=VALUES(cycle_id),"
            + "projected_at=CURRENT_TIMESTAMP(6)"
        )
        parameters = tuple(value for record in batch.records for value in _record_parameters(record, cycle_id))
        self._execute(sql, parameters)

    def upsert_state(self, state: ProjectionState) -> None:
        parameters = _state_parameters(state)
        placeholders = ",".join(["%s"] * len(STATE_COLUMNS))
        updates = ",".join(
            f"{column}=VALUES({column})" for column in STATE_COLUMNS[1:]
        )
        self._execute(
            "INSERT INTO telemetry_reward_projection_state (" + ",".join(STATE_COLUMNS) + ") VALUES ("
            + placeholders + ") ON DUPLICATE KEY UPDATE " + updates,
            parameters,
        )

    def process_page(self, adapter: SourceAdapter, state: ProjectionState, *,
                     high_water_usec: int, reconciliation: bool,
                     page_complete: bool | None = None) -> tuple[ProjectionBatch, ProjectionState]:
        """Read, project, idempotently write, and advance one bounded page."""

        state.validate()
        cursor = state.reconcile_cursor if reconciliation else state.fast_cursor
        if high_water_usec < cursor.created_at_usec:
            raise ValueError("high-water timestamp cannot precede the selected cursor")
        self._statement_count = 0
        started_at_usec = _utc_now_usec()
        if reconciliation:
            active_state = replace(
                state,
                reconcile_started_at_usec=(
                    state.reconcile_started_at_usec or started_at_usec
                ),
            )
        else:
            active_state = replace(state, fast_started_at_usec=started_at_usec)
        self._begin()
        try:
            observations = self.fetch_page(adapter, cursor=cursor, high_water_usec=high_water_usec)
            batch = project_source_page(observations)
            next_cursor = cursor
            if observations:
                last = observations[-1]
                next_cursor = SourceCursor(
                    last.created_at_usec, normalize_operation_id(last.operation_id),
                    last.entry_index, last.participant_pid,
                )
            complete = len(observations) < self.page_size if page_complete is None else page_complete
            next_state = active_state.acknowledge_page(
                cursor=next_cursor,
                rows_seen=len(observations),
                reconciliation=reconciliation,
                page_complete=complete,
            )
            if reconciliation and complete:
                next_state = replace(next_state, reconcile_completed_at_usec=_utc_now_usec())
            elif not reconciliation:
                next_state = replace(next_state, fast_completed_at_usec=_utc_now_usec())
            next_state.validate()
            self.upsert_projection(batch, cycle_id=next_state.cycle_id)
            self.upsert_state(next_state)
            self._commit()
            return batch, next_state
        except Exception:
            self._rollback()
            raise

    def read_report(self, *, start_usec: int, end_usec: int,
                    max_rows: int = MAX_PAGE_SIZE) -> dict[str, Any]:
        """Read bounded reward totals and coverage metadata on this one connection."""

        self._statement_count = 0
        report_query, parameters = build_reward_report_query(
            start_usec=start_usec, end_usec=end_usec, max_rows=max_rows
        )
        rows = self._execute(report_query, parameters)
        state_rows = self._execute(build_reward_state_query())
        return {
            "definition": dict(REWARD_REPORT_DEFINITION),
            "rows": rows,
            "projection_state": state_rows,
        }


def adapter_catalog() -> list[dict[str, Any]]:
    return [
        {
            "name": adapter.name,
            "source_kind": int(adapter.source_kind),
            "table": adapter.table,
            "supported": adapter.supported,
            "context_only": adapter.context_only,
            "description": adapter.description,
        }
        for adapter in SOURCE_ADAPTERS
    ]


def _connection_factory_from_env() -> ConnectionFactory:
    try:
        from .db_access import ConnectionSettings, PyMySQLConnectionFactory
    except ImportError:
        from db_access import ConnectionSettings, PyMySQLConnectionFactory  # type: ignore[no-redef]
    return PyMySQLConnectionFactory(ConnectionSettings.from_env("TELEMETRY_REWARD_DB_"))


def _parser() -> ArgumentParser:
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", action="store_true", help="print the explicit source adapter catalog")
    parser.add_argument("--source", choices=sorted(SOURCE_ADAPTER_BY_NAME), help="one source adapter")
    parser.add_argument("--high-water-usec", type=int, help="fixed UTC high-water timestamp for one page")
    parser.add_argument("--cycle-id", type=int, default=0)
    parser.add_argument("--reconcile", action="store_true")
    parser.add_argument("--retention-floor-usec", type=int)
    parser.add_argument("--cursor-created-at-usec", type=int, default=0)
    parser.add_argument("--cursor-operation-id", default=ZERO_OPERATION_ID.hex())
    parser.add_argument("--cursor-entry-index", type=int, default=0)
    parser.add_argument("--cursor-participant-pid", type=int, default=0)
    parser.add_argument("--page-size", type=int, default=256)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    if args.catalog:
        print(json.dumps(adapter_catalog(), sort_keys=True, indent=2))
        return 0
    if not args.source or args.high_water_usec is None:
        _parser().error("--source and --high-water-usec are required unless --catalog is used")
    adapter = SOURCE_ADAPTER_BY_NAME[args.source]
    if not adapter.supported:
        raise UnsupportedSourceError(adapter.description)
    operation_id = normalize_operation_id(args.cursor_operation_id) if args.cursor_operation_id != ZERO_OPERATION_ID.hex() else ZERO_OPERATION_ID
    cursor = SourceCursor(
        args.cursor_created_at_usec, operation_id, args.cursor_entry_index, args.cursor_participant_pid
    )
    state = ProjectionState(
        source_kind=adapter.source_kind,
        cycle_id=args.cycle_id,
        reconcile_cursor=cursor,
        fast_cursor=cursor,
        cycle_high_water_usec=args.high_water_usec,
    )
    if args.reconcile:
        if args.cycle_id <= 0 or args.retention_floor_usec is None:
            _parser().error("reconciliation requires positive --cycle-id and --retention-floor-usec")
        state = state.start_reconciliation(
            cycle_id=args.cycle_id,
            retention_floor_usec=args.retention_floor_usec,
            high_water_usec=args.high_water_usec,
        )
    store = RewardProjectionStore(_connection_factory_from_env(), page_size=args.page_size)
    try:
        batch, next_state = store.process_page(
            adapter, state, high_water_usec=args.high_water_usec,
            reconciliation=args.reconcile,
        )
        print(json.dumps({
            "source": adapter.name,
            "records": len(batch.records),
            "duplicates": batch.duplicate_count,
            "conflicts": batch.conflict_count,
            "gross_total": batch.gross_total,
            "net_total": batch.net_total,
            "transfer_total": batch.transfer_total,
            "provisional": next_state.provisional,
            "backlog_rows": next_state.backlog_rows,
        }, sort_keys=True))
    finally:
        store.close()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RewardProjectionError as error:
        print(f"reward projection failed: {error}", file=sys.stderr)
        raise SystemExit(2)


__all__ = [
    "AmbiguousCommit", "PROJECTION_INSERT_COLUMNS", "STATE_COLUMNS", "ProjectionBoundsExceeded",
    "RewardProjectionError", "RewardProjectionStore",
    "SOURCE_ADAPTERS", "SOURCE_ADAPTER_BY_NAME", "SUPPORTED_SOURCE_ADAPTERS",
    "SourceAdapter", "UnsupportedSourceError", "adapter_catalog",
    "build_reward_report_query", "build_reward_state_query", "build_source_page_query",
    "main", "project_source_page",
]
