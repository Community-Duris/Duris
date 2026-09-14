"""Restricted one-connection PyMySQL boundary for the telemetry rollup engine.

This module never runs migrations and never writes the immutable raw fact stream.
All SQL identifiers are repository constants; caller values are bound parameters.
PyMySQL is imported only when a real connection is requested, so pure rollup
semantics remain dependency-free.
"""
from __future__ import annotations

from dataclasses import dataclass
import ipaddress
import hashlib
import os
import re
import threading
import time
import math
from typing import Any, Callable, Mapping, Sequence

try:  # Running as a package.
    from .rollup_definitions import (
        COUNTER_FIELDS,
        CheckpointContribution,
        MembershipContribution,
        PUBLICATION_BUILDING,
        PUBLICATION_FAILED,
        PUBLICATION_PUBLISHED,
        PUBLICATION_SUPERSEDED,
        ReportSnapshot,
        RollupCoverage,
        RollupTarget,
        UNKNOWN_DAY,
        report_definition,
    )
    from .rollup_engine import (
        BoundsExceeded,
        CursorError,
        MAX_RETRIES_HARD_MAX,
        PageContribution,
        REPORT_BYTE_LIMIT_DEFAULT,
        REPORT_ROW_LIMIT_DEFAULT,
        REPORT_RUNTIME_DEFAULT_S,
        RollupBounds,
        RollupError,
        RollupPageResult,
        SemanticError,
        build_page_contributions,
        checkpoint_contribution_from_row,
        coverage_from_state_row,
        estimate_row_bytes,
        membership_contribution_from_row,
        merge_session_projection,
    )
except ImportError:  # Running scripts/telemetry/rollup.py directly.
    from rollup_definitions import (  # type: ignore[no-redef]
        COUNTER_FIELDS,
        CheckpointContribution,
        MembershipContribution,
        PUBLICATION_BUILDING,
        PUBLICATION_FAILED,
        PUBLICATION_PUBLISHED,
        PUBLICATION_SUPERSEDED,
        ReportSnapshot,
        RollupCoverage,
        RollupTarget,
        UNKNOWN_DAY,
        report_definition,
    )
    from rollup_engine import (  # type: ignore[no-redef]
        BoundsExceeded,
        CursorError,
        MAX_RETRIES_HARD_MAX,
        PageContribution,
        REPORT_BYTE_LIMIT_DEFAULT,
        REPORT_ROW_LIMIT_DEFAULT,
        REPORT_RUNTIME_DEFAULT_S,
        RollupBounds,
        RollupError,
        RollupPageResult,
        SemanticError,
        build_page_contributions,
        checkpoint_contribution_from_row,
        coverage_from_state_row,
        estimate_row_bytes,
        membership_contribution_from_row,
        merge_session_projection,
    )


class DatabaseAccessError(RollupError):
    """A database operation could not complete and made no acknowledged page."""


class AmbiguousCommit(DatabaseAccessError):
    """COMMIT acknowledgement was lost; the cursor must be reread first."""


class GenerationConflict(DatabaseAccessError):
    """A stale generation attempted to supersede a newer published generation."""


class LockTimeout(DatabaseAccessError):
    """The bounded advisory lock was not acquired."""


class ConcurrentConnectionError(DatabaseAccessError):
    """The restricted factory was asked for a second live connection."""


_LOOPBACK_NAMES = frozenset({"localhost", "127.0.0.1", "::1"})
_IDENTIFIER_RE = re.compile(r"^[A-Za-z0-9_$-]+$")
_CONNECTING = object()


@dataclass(frozen=True, slots=True)
class ConnectionSettings:
    """Explicit external rollup connection settings.

    The password is accepted only from the environment by ``from_env`` or by
    an in-process caller.  It is never included in repr/log output.
    """

    host: str
    database: str
    user: str
    password: str
    port: int = 3306
    tls_ca: str | None = None
    tls_verify: bool = False
    secure_tunnel: bool = False
    connect_timeout_s: float = 2.0
    read_timeout_s: float = 3.0
    write_timeout_s: float = 3.0
    statement_timeout_s: float = 2.0
    lock_timeout_s: float = 2.0

    def __repr__(self) -> str:  # Do not accidentally print credentials.
        return (
            "ConnectionSettings(host={!r}, database={!r}, user={!r}, port={!r}, "
            "tls_verify={!r}, secure_tunnel={!r})"
        ).format(
            self.host,
            self.database,
            self.user,
            self.port,
            self.tls_verify,
            self.secure_tunnel,
        )

    @property
    def is_loopback(self) -> bool:
        host = self.host.strip().lower()
        if host in _LOOPBACK_NAMES:
            return True
        try:
            return ipaddress.ip_address(host).is_loopback
        except ValueError:
            return False

    def validate(self) -> None:
        if not isinstance(self.host, str) or not self.host.strip():
            raise ValueError("rollup database host must be explicit")
        if not isinstance(self.database, str) or not self.database or not _IDENTIFIER_RE.fullmatch(self.database):
            raise ValueError("rollup database name must be an explicit safe identifier")
        if not isinstance(self.user, str) or not self.user.strip():
            raise ValueError("rollup database user must be explicit")
        if not isinstance(self.password, str):
            raise ValueError("rollup database password must come from the environment")
        if isinstance(self.port, bool) or not isinstance(self.port, int) or not 1 <= self.port <= 65535:
            raise ValueError("rollup database port must be 1..65535")
        for name in (
            "connect_timeout_s",
            "read_timeout_s",
            "write_timeout_s",
            "statement_timeout_s",
            "lock_timeout_s",
        ):
            value = getattr(self, name)
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
                raise ValueError(f"{name} must be a positive number")
        if self.tls_verify and (not self.tls_ca or not os.path.isfile(self.tls_ca)):
            raise ValueError("verified TLS requires an existing CA file")
        if not self.is_loopback and not self.tls_verify and not self.secure_tunnel:
            raise ValueError(
                "non-loopback rollup connections require verified TLS or explicit secure_tunnel mode"
            )
        if self.secure_tunnel and self.tls_verify:
            # Explicit tunnel mode may still use TLS, but the CA remains the
            # authority.  There is no silent downgrade.
            return

    @classmethod
    def from_env(cls, prefix: str = "TELEMETRY_ROLLUP_DB_") -> "ConnectionSettings":
        """Read only the dedicated rollup namespace, never the game ``DB_*`` set."""

        required = ("HOST", "DATABASE", "USER")
        missing = [name for name in required if not os.environ.get(prefix + name)]
        password = os.environ.get(prefix + "PASSWORD")
        if password is None:
            password = os.environ.get(prefix + "PASSWD")
        if password is None:
            missing.append("PASSWORD")
        if missing:
            raise ValueError(
                "missing explicit rollup environment values: "
                + ", ".join(prefix + name for name in missing)
            )

        def boolean(name: str, default: bool) -> bool:
            value = os.environ.get(prefix + name)
            if value is None:
                return default
            normalized = value.strip().lower()
            if normalized not in {"0", "1", "false", "true", "no", "yes"}:
                raise ValueError(f"{prefix + name} must be true/false")
            return normalized in {"1", "true", "yes"}

        def number(name: str, default: float) -> float:
            value = os.environ.get(prefix + name)
            if value is None:
                return default
            try:
                return float(value)
            except ValueError as error:
                raise ValueError(f"{prefix + name} must be numeric") from error

        settings = cls(
            host=os.environ[prefix + "HOST"],
            database=os.environ[prefix + "DATABASE"],
            user=os.environ[prefix + "USER"],
            password=password,
            port=int(os.environ.get(prefix + "PORT", "3306")),
            tls_ca=os.environ.get(prefix + "SSL_CA"),
            tls_verify=boolean("TLS_VERIFY", False),
            secure_tunnel=boolean("SECURE_TUNNEL", False),
            connect_timeout_s=number("CONNECT_TIMEOUT_S", 2.0),
            read_timeout_s=number("READ_TIMEOUT_S", 3.0),
            write_timeout_s=number("WRITE_TIMEOUT_S", 3.0),
            statement_timeout_s=number("STATEMENT_TIMEOUT_S", 2.0),
            lock_timeout_s=number("LOCK_TIMEOUT_S", 2.0),
        )
        settings.validate()
        return settings


class PyMySQLConnectionFactory:
    """A serial connection factory with no pool and no credential logging."""

    def __init__(self, settings: ConnectionSettings) -> None:
        settings.validate()
        self.settings = settings
        self._active_connection: Any | None = None
        self._state_lock = threading.Lock()

    @property
    def active(self) -> bool:
        with self._state_lock:
            return self._active_connection is not None

    def connect(self) -> Any:
        with self._state_lock:
            if self._active_connection is not None:
                raise ConcurrentConnectionError("rollup factory permits one live connection only")
            self._active_connection = _CONNECTING
        try:
            import pymysql  # lazy optional dependency
        except ImportError as error:
            with self._state_lock:
                self._active_connection = None
            raise DatabaseAccessError(
                "PyMySQL is required only for the real rollup CLI/adapter"
            ) from error
        kwargs: dict[str, Any] = {
            "host": self.settings.host,
            "port": self.settings.port,
            "user": self.settings.user,
            "password": self.settings.password,
            "database": self.settings.database,
            "charset": "utf8mb4",
            "autocommit": False,
            "connect_timeout": max(1, int(self.settings.connect_timeout_s)),
            "read_timeout": max(1, int(self.settings.read_timeout_s)),
            "write_timeout": max(1, int(self.settings.write_timeout_s)),
            "cursorclass": pymysql.cursors.DictCursor,
        }
        if self.settings.tls_verify:
            kwargs["ssl"] = {
                "ca": self.settings.tls_ca,
                "check_hostname": True,
            }
        try:
            connection = pymysql.connect(**kwargs)
            self._configure_session(connection)
        except Exception as error:
            try:
                connection.close()  # type: ignore[has-type]
            except Exception:
                pass
            with self._state_lock:
                self._active_connection = None
            if isinstance(error, RollupError):
                raise
            raise DatabaseAccessError("rollup database connection/session setup failed") from error
        self._active_connection = connection
        return connection

    def _configure_session(self, connection: Any) -> None:
        """Apply UTC, isolation, lock, and server-specific statement limits."""

        cursor = connection.cursor()
        try:
            cursor.execute("SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED")
            cursor.execute("SET SESSION time_zone = '+00:00'")
            lock_seconds = max(1, int(self.settings.lock_timeout_s + 0.999999))
            cursor.execute("SET SESSION innodb_lock_wait_timeout = %s", (lock_seconds,))
            server_info = ""
            getter = getattr(connection, "get_server_info", None)
            if getter is not None:
                server_info = str(getter())
            if "mariadb" in server_info.lower():
                cursor.execute(
                    "SET SESSION max_statement_time = %s",
                    (float(self.settings.statement_timeout_s),),
                )
            else:
                cursor.execute(
                    "SET SESSION MAX_EXECUTION_TIME = %s",
                    (max(1, int(self.settings.statement_timeout_s * 1000)),),
                )
        finally:
            cursor.close()

    def close(self, connection: Any | None = None) -> None:
        connection = connection if connection is not None else self._active_connection
        if connection is None:
            return
        try:
            connection.close()
        finally:
            if connection is self._active_connection:
                self._active_connection = None


# Explicit raw projection.  No SELECT * means schema drift cannot silently add
# unbounded blobs/columns to a page.
RAW_COLUMNS = (
    "ingest_id",
    "boot_id",
    "process_id",
    "record_seq",
    "schema_version",
    "record_kind",
    "occurrence_utc_usec",
    "ingested_utc_usec",
    "environment_id",
    "season_id",
    "session_boot_id",
    "session_process_id",
    "session_seq",
    "subject_id",
    "pid",
    "connection_boot_id",
    "connection_process_id",
    "connection_seq",
    "start_monotonic_usec",
    "end_monotonic_usec",
    "start_utc_usec",
    "end_utc_usec",
    "duration_usec",
    "category",
    "context",
    "context_quality",
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "group_size",
    "config_id",
    "classifier_version",
    "policy_version",
    "quality_flags",
    "lifecycle",
    "end_reason",
    "at_monotonic_usec",
    "at_utc_usec",
    "checkpoint_revision",
    "connected_usec",
    "active_usec",
    "idle_usec",
    "unknown_usec",
    "resident_usec",
    "linkdead_usec",
    "gap_reason",
    "first_missing_record_seq",
    "last_missing_record_seq",
    "dropped_records",
    "config_revision",
    "build_version",
    "content_version",
    "property_version",
    "fingerprint",
    "effective_utc_usec",
    "interval_usec",
    "checkpoint_interval_usec",
    "active_window_usec",
    "context_segments_per_minute",
    "pulse_slot_count",
    "backend",
    "enabled",
)

SESSION_COLUMNS = (
    "definition_version",
    "generation",
    "environment_id",
    "season_id",
    "session_boot_id",
    "session_process_id",
    "session_seq",
    "subject_id",
    "pid",
    "latest_checkpoint_revision",
    *COUNTER_FIELDS,
    *("covered_" + name for name in COUNTER_FIELDS),
    "attributable_usec",
    "observed_intervals",
    "entered",
    "exited",
    "end_reason",
    "quality_flags",
    "input_watermark",
    "provisional",
)
PLAYER_DAY_COLUMNS = (
    "definition_version",
    "generation",
    "environment_id",
    "season_id",
    "utc_day",
    "subject_id",
    "session_boot_id",
    "session_process_id",
    "session_seq",
    *COUNTER_FIELDS,
    "attributable_usec",
    "observed_intervals",
    "quality_flags",
    "input_watermark",
    "provisional",
)
COHORT_COLUMNS = (
    "definition_version",
    "generation",
    "environment_id",
    "season_id",
    "utc_day",
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "config_id",
    "category",
    "duration_usec",
    "attributable_usec",
    "observed_intervals",
    "subject_count",
    "session_count",
    "quality_flags",
    "input_watermark",
    "provisional",
)
MEMBER_COLUMNS = (
    "definition_version",
    "generation",
    "environment_id",
    "season_id",
    "utc_day",
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "config_id",
    "category",
    "membership_kind",
    "subject_id",
    "session_boot_id",
    "session_process_id",
    "session_seq",
    "duration_usec",
    "attributable_usec",
    "observed_intervals",
    "quality_flags",
    "input_watermark",
)
STATE_COLUMNS = (
    "definition_version",
    "generation",
    "environment_id",
    "season_id",
    "input_watermark",
    "publication_status",
    "coverage_start_utc_usec",
    "coverage_end_utc_usec",
    "quality_flags",
    "provisional",
    "rebuild_from_ingest_id",
    "rebuild_through_ingest_id",
)

SCOPE_WHERE = "definition_version=%s AND generation=%s AND environment_id=%s AND season_id=%s"
SESSION_KEY_WHERE = SCOPE_WHERE + " AND session_boot_id=%s AND session_process_id=%s AND session_seq=%s"
COHORT_KEY_WHERE = (
    SCOPE_WHERE
    + " AND utc_day=%s AND level_band=%s AND class_id=%s AND race_id=%s"
    + " AND faction_id=%s AND zone_vnum=%s AND config_id=%s AND category=%s"
)
MEMBER_KEY_WHERE = (
    COHORT_KEY_WHERE
    + " AND membership_kind=%s AND subject_id=%s AND session_boot_id=%s"
    + " AND session_process_id=%s AND session_seq=%s"
)

REPORT_TABLES = {
    "session_playtime": ("telemetry_rollup_session", SESSION_COLUMNS),
    "cohort_activity": ("telemetry_cohort_day", COHORT_COLUMNS),
}
REPORT_ORDER_BY = {
    "session_playtime": (
        "definition_version,generation,environment_id,season_id,"
        "subject_id,session_boot_id,session_process_id,session_seq"
    ),
    "cohort_activity": (
        "definition_version,generation,environment_id,season_id,utc_day,"
        "level_band,class_id,race_id,faction_id,zone_vnum,config_id,category"
    ),
}
REPORT_ROW_LIMIT_DEFAULT = 10_000
REPORT_ROW_LIMIT_HARD_MAX = 100_000
# Aggregate rows contain only bounded integer/date columns.  Reserve a
# deliberately conservative fixed-width envelope before asking the driver to
# materialize a row; one extra row is reserved as the truncation sentinel.
REPORT_BYTE_LIMIT_HARD_MAX = 256 * 1024 * 1024
REPORT_ROW_BYTE_BOUND = 2_048
REPORT_MIN_FETCH_ROWS = 2


# Preserve immutable0014's exact one-index state schema. Publication performs
# a bounded PRIMARY-prefix scan, then point updates, never an unbounded catalog
# enumeration or a broad UPDATE over unindexed environment/status predicates.
PUBLICATION_STATE_LIMIT = 4096


def _bounded_report_limit(value: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise ValueError("report max_rows must be a positive integer")
    if value > REPORT_ROW_LIMIT_HARD_MAX:
        raise ValueError(f"report max_rows exceeds hard maximum {REPORT_ROW_LIMIT_HARD_MAX}")
    return value


def _bounded_report_bytes(value: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise ValueError("report max_bytes must be a positive integer")
    if value > REPORT_BYTE_LIMIT_HARD_MAX:
        raise ValueError(f"report max_bytes exceeds hard maximum {REPORT_BYTE_LIMIT_HARD_MAX}")
    return value


def _bounded_report_runtime(value: float) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
        raise ValueError("report max_runtime_s must be a positive number")
    if value > 3_600:
        raise ValueError("report max_runtime_s exceeds hard maximum")
    return float(value)


def _report_fetch_limits(max_rows: int, max_bytes: int) -> tuple[int, int]:
    """Return result and SQL fetch limits without exceeding the byte budget."""

    row_limit = _bounded_report_limit(max_rows)
    byte_limit = _bounded_report_bytes(max_bytes)
    capacity = byte_limit // REPORT_ROW_BYTE_BOUND
    if capacity < REPORT_MIN_FETCH_ROWS:
        raise BoundsExceeded(
            "report byte budget cannot reserve one result row and a truncation sentinel"
        )
    result_limit = min(row_limit, capacity - 1)
    return result_limit, result_limit + 1


def _report_rows_bytes(rows: Sequence[Mapping[str, Any]]) -> int:
    return sum(estimate_row_bytes(row) for row in rows)


def _validate_cursor_value(value: int, name: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0 or value > (1 << 64) - 1:
        raise ValueError(f"{name} must be an unsigned 64-bit integer")
    return value


@dataclass(frozen=True, slots=True)
class _PageMaterial:
    contribution: PageContribution
    start_cursor: int
    page_last_ingest_id: int
    rows_fetched: int
    estimated_bytes: int


class PyMySQLRollupDatabase:
    """One-connection implementation of the structural rollup adapter."""

    def __init__(
        self,
        connection_factory: PyMySQLConnectionFactory,
        *,
        max_commit_retries: int = 2,
        clock: Callable[[], float] = time.monotonic,
    ) -> None:
        if isinstance(max_commit_retries, bool) or not isinstance(max_commit_retries, int) or max_commit_retries < 0:
            raise ValueError("max_commit_retries must be a nonnegative integer")
        if max_commit_retries > MAX_RETRIES_HARD_MAX:
            raise ValueError(f"max_commit_retries exceeds hard maximum {MAX_RETRIES_HARD_MAX}")
        self.connection_factory = connection_factory
        self.max_commit_retries = max_commit_retries
        self.clock = clock
        self._connection: Any | None = None
        self._statement_count: int | None = None
        self._statement_limit: int | None = None
        self._deadline: float | None = None
        self._held_lock_name: str | None = None
        self._active_page_deadline: float | None = None

    @property
    def connected(self) -> bool:
        return self._connection is not None

    def close(self) -> None:
        self._release_advisory_lock()
        if self._connection is not None:
            self._drop_connection()

    def _ensure_connection(self) -> Any:
        if self._connection is None:
            self._connection = self.connection_factory.connect()
        return self._connection

    def _drop_connection(self) -> None:
        if self._connection is None:
            return
        connection = self._connection
        try:
            try:
                self.connection_factory.close(connection)
            except Exception:
                # Keep the fail-closed boundary usable by small connector fakes
                # and by a partially initialized real connection.
                try:
                    connection.close()
                except Exception:
                    pass
        finally:
            self._connection = None
            self._held_lock_name = None

    def _check_deadline(self) -> None:
        if self._deadline is not None and self.clock() >= self._deadline:
            raise BoundsExceeded("rollup database operation exceeded its monotonic deadline")

    def _check_budget(self) -> None:
        self._check_deadline()
        if self._statement_count is not None:
            self._statement_count += 1
            if self._statement_limit is not None and self._statement_count > self._statement_limit:
                raise BoundsExceeded("rollup transaction statement bound exceeded")

    def _execute(self, statement: str, parameters: Sequence[Any] = ()) -> tuple[list[Mapping[str, Any]], int, Any]:
        self._check_budget()
        connection = self._ensure_connection()
        cursor = connection.cursor()
        try:
            cursor.execute(statement, tuple(parameters))
            description = getattr(cursor, "description", None)
            rows = list(cursor.fetchall()) if description else []
            # Socket/server settings can stop a query, but they cannot prove
            # that a connector call returned before our invocation deadline.
            # Check again after materialization and discard the connection if it
            # crossed the boundary.
            self._check_deadline()
            return rows, int(getattr(cursor, "rowcount", 0) or 0), cursor
        except BoundsExceeded:
            self._drop_connection()
            raise
        except Exception as error:
            if isinstance(error, RollupError):
                raise
            raise DatabaseAccessError("rollup SQL statement failed") from error
        finally:
            try:
                cursor.close()
            except Exception:
                pass

    def _begin(self) -> None:
        connection = self._ensure_connection()
        begin = getattr(connection, "begin", None)
        if begin is not None:
            self._check_budget()
            begin()
        else:
            self._execute("START TRANSACTION")

    def _rollback(self) -> None:
        if self._connection is None:
            return
        try:
            self._connection.rollback()
        except Exception:
            self._drop_connection()

    def _commit(self) -> None:
        connection = self._ensure_connection()
        # No COMMIT was sent if the local budget check fails: this is a
        # definite rollback path, not an acknowledgement ambiguity.
        self._check_budget()
        try:
            connection.commit()
        except Exception as error:
            # A failed acknowledgement is ambiguous even when the driver gives
            # a generic exception.  The caller must discard this socket.
            raise AmbiguousCommit("rollup COMMIT acknowledgement is ambiguous") from error
        self._check_commit_deadline()

    def _check_commit_deadline(self) -> None:
        """Turn a late successful COMMIT call into an ambiguity."""

        try:
            self._check_deadline()
        except BoundsExceeded as error:
            self._drop_connection()
            raise AmbiguousCommit("rollup COMMIT completed after its deadline") from error

    def _fetch_state(self, target: RollupTarget, *, for_update: bool) -> Mapping[str, Any] | None:
        lock = " FOR UPDATE" if for_update else ""
        rows, _count, _ = self._execute(
            "SELECT "
            + ",".join(STATE_COLUMNS)
            + " FROM telemetry_rollup_state WHERE "
            + SCOPE_WHERE
            + lock,
            target.scope_tuple,
        )
        return rows[0] if rows else None

    def _ensure_state_locked(
        self,
        target: RollupTarget,
        through_ingest_id: int,
        origin_ingest_id: int | None,
    ) -> Mapping[str, Any]:
        initial_origin = 0 if origin_ingest_id is None else origin_ingest_id
        if through_ingest_id < initial_origin:
            raise ValueError("through_ingest_id must be at least origin_ingest_id")
        self._execute(
            "INSERT INTO telemetry_rollup_state ("
            + ",".join(STATE_COLUMNS)
            + ") VALUES (" + ",".join(["%s"] * len(STATE_COLUMNS)) + ") "
            "ON DUPLICATE KEY UPDATE definition_version=VALUES(definition_version)",
            (
                target.definition_version,
                target.generation,
                target.environment_id,
                target.season_id,
                initial_origin,
                PUBLICATION_BUILDING,
                None,
                None,
                0,
                1,
                initial_origin,
                through_ingest_id,
            ),
        )
        row = self._fetch_state(target, for_update=True)
        if row is None:
            raise DatabaseAccessError("rollup state row disappeared after bootstrap")
        stored_origin = int(row["rebuild_from_ingest_id"])
        if origin_ingest_id is not None and stored_origin != origin_ingest_id:
            raise GenerationConflict(
                f"generation origin is immutable: stored={stored_origin}, requested={origin_ingest_id}"
            )
        status = int(row["publication_status"])
        if status in {PUBLICATION_FAILED, PUBLICATION_SUPERSEDED}:
            raise GenerationConflict("non-active rollup generation requires a new explicit generation")
        stored_through = int(row["rebuild_through_ingest_id"])
        if origin_ingest_id is not None and through_ingest_id < stored_through:
            raise GenerationConflict(
                f"generation high-water bound cannot regress: stored={stored_through}, requested={through_ingest_id}"
            )
        if through_ingest_id > stored_through:
            self._execute(
                "UPDATE telemetry_rollup_state SET rebuild_through_ingest_id=%s WHERE " + SCOPE_WHERE,
                (through_ingest_id, *target.scope_tuple),
            )
            row = self._fetch_state(target, for_update=True)
            if row is None:
                raise DatabaseAccessError("rollup state row disappeared after bound update")
        return row

    def snapshot_high_watermark(self) -> int:
        rows, _count, _ = self._execute(
            "SELECT COALESCE(MAX(ingest_id),0) AS snapshot_high_watermark FROM telemetry_interval"
        )
        if not rows:
            raise DatabaseAccessError("raw telemetry high-water query returned no row")
        value = int(rows[0]["snapshot_high_watermark"])
        if value < 0:
            raise CursorError("raw telemetry high-watermark regressed")
        return value

    def _fetch_raw_page(self, cursor: int, through_ingest_id: int, limit: int) -> list[Mapping[str, Any]]:
        _validate_cursor_value(cursor, "cursor")
        _validate_cursor_value(through_ingest_id, "through_ingest_id")
        if isinstance(limit, bool) or not isinstance(limit, int) or limit <= 0:
            raise BoundsExceeded("raw page limit must be positive")
        statement = (
            "SELECT "
            + ",".join(RAW_COLUMNS)
            + " FROM telemetry_interval FORCE INDEX (PRIMARY)"
            " WHERE ingest_id>%s AND ingest_id<=%s ORDER BY ingest_id LIMIT %s"
        )
        rows, _count, _ = self._execute(statement, (cursor, through_ingest_id, limit))
        return rows

    def _advisory_lock_name(self, target: RollupTarget) -> str:
        scope = (f"duris.telemetry.rollup.v1|{self.connection_factory.settings.database}|"
                 f"{target.definition_version}|{target.environment_id}|{target.season_id}")
        return hashlib.sha256(scope.encode("utf-8")).hexdigest()

    def _acquire_advisory_lock(self, target: RollupTarget, timeout_s: float) -> None:
        name = self._advisory_lock_name(target)
        timeout = max(0.001, float(timeout_s))
        rows, _count, _ = self._execute("SELECT GET_LOCK(%s,%s) AS acquired", (name, timeout))
        if not rows or int(rows[0].get("acquired", 0) or 0) != 1:
            raise LockTimeout("bounded rollup advisory lock was not acquired")
        self._held_lock_name = name

    def _release_advisory_lock(self) -> None:
        if self._held_lock_name is None or self._connection is None:
            self._held_lock_name = None
            return
        name = self._held_lock_name
        self._held_lock_name = None
        try:
            self._execute("SELECT RELEASE_LOCK(%s) AS released", (name,))
        except Exception:
            # A budget expiry can prevent RELEASE_LOCK from reaching the
            # server. Discard the socket in that case so a reused adapter cannot
            # retain an untracked advisory lock and starve later jobs.
            self._drop_connection()

    def _prepare_transaction_budget(self, bounds: RollupBounds) -> None:
        self._statement_count = 0
        self._statement_limit = bounds.max_transaction_statements
        self._deadline = self.clock() + float(bounds.max_runtime_s)
        if self._active_page_deadline is not None:
            self._deadline = min(self._deadline, self._active_page_deadline)
        remaining = self._deadline - self.clock()
        if remaining <= 0:
            raise BoundsExceeded("fixed page deadline expired before transaction setup")
        connection = self._ensure_connection()
        settings = self.connection_factory.settings
        socket_limit = min(float(bounds.socket_timeout_s), remaining)
        # PyMySQL applies these to each socket operation (not only connect()).
        connection._read_timeout = min(settings.read_timeout_s, socket_limit)
        connection._write_timeout = min(settings.write_timeout_s, socket_limit)
        statement_limit = min(settings.statement_timeout_s, float(bounds.statement_timeout_s), remaining)
        server_info = str(connection.get_server_info()).lower()
        if "mariadb" in server_info:
            self._execute("SET SESSION max_statement_time=%s", (statement_limit,))
        else:
            self._execute("SET SESSION MAX_EXECUTION_TIME=%s", (max(1, int(statement_limit * 1000)),))
        # InnoDB's lock setting has whole-second granularity; socket/deadline
        # checks additionally bound sub-second caller policies.
        self._execute("SET SESSION innodb_lock_wait_timeout=%s",
                      (max(1, math.ceil(min(settings.lock_timeout_s, bounds.lock_timeout_s))),))

    def _clear_transaction_budget(self) -> None:
        self._statement_count = None
        self._statement_limit = None
        self._deadline = None

    def _prepare_report_budget(
        self,
        max_runtime_s: float,
        max_bytes: int,
        *,
        reserve_sentinel: bool,
    ) -> None:
        """Install a bounded read budget before any report SQL is issued."""

        runtime = _bounded_report_runtime(max_runtime_s)
        byte_limit = _bounded_report_bytes(max_bytes)
        minimum_rows = REPORT_MIN_FETCH_ROWS if reserve_sentinel else 1
        if byte_limit // REPORT_ROW_BYTE_BOUND < minimum_rows:
            if reserve_sentinel:
                raise BoundsExceeded(
                    "report byte budget cannot reserve one result row and a truncation sentinel"
                )
            raise BoundsExceeded("report byte budget cannot reserve one bounded row")
        self._statement_count = 0
        self._statement_limit = 16
        self._deadline = self.clock() + runtime
        try:
            connection = self._ensure_connection()
            self._check_deadline()
            settings = self.connection_factory.settings
            remaining = self._deadline - self.clock()
            socket_limit = max(0.001, min(float(settings.read_timeout_s), remaining))
            connection._read_timeout = socket_limit
            connection._write_timeout = max(0.001, min(float(settings.write_timeout_s), remaining))
        except Exception:
            self._drop_connection()
            self._clear_transaction_budget()
            raise

    def _write_session(self, projected: Mapping[str, Any]) -> None:
        values = tuple(projected[name] for name in SESSION_COLUMNS)
        update_columns = tuple(name for name in SESSION_COLUMNS if name not in {
            "definition_version", "generation", "environment_id", "season_id",
            "session_boot_id", "session_process_id", "session_seq",
        })
        update = ",".join(f"{name}=VALUES({name})" for name in update_columns)
        self._execute(
            "INSERT INTO telemetry_rollup_session ("
            + ",".join(SESSION_COLUMNS)
            + ") VALUES (" + ",".join(["%s"] * len(SESSION_COLUMNS)) + ") "
            "ON DUPLICATE KEY UPDATE "
            + update,
            values,
        )

    def _read_locked_session(
        self,
        target: RollupTarget,
        key: tuple[int, int, int],
    ) -> Mapping[str, Any] | None:
        rows, _count, _ = self._execute(
            "SELECT " + ",".join(SESSION_COLUMNS)
            + " FROM telemetry_rollup_session WHERE " + SESSION_KEY_WHERE + " FOR UPDATE",
            (*target.scope_tuple, *key),
        )
        return rows[0] if rows else None

    def _write_player_day(self, delta: Any) -> None:
        values: dict[str, Any] = {
            "definition_version": delta.target.definition_version,
            "generation": delta.target.generation,
            "environment_id": delta.target.environment_id,
            "season_id": delta.target.season_id,
            "utc_day": delta.utc_day,
            "subject_id": delta.subject_id,
            "session_boot_id": delta.session_boot_id,
            "session_process_id": delta.session_process_id,
            "session_seq": delta.session_seq,
            **delta.counters,
            "attributable_usec": delta.attributable_usec,
            "observed_intervals": delta.observed_intervals,
            "quality_flags": delta.quality_flags,
            "input_watermark": delta.input_watermark,
            "provisional": 1,
        }
        update_columns = (
            *COUNTER_FIELDS,
            "attributable_usec",
            "observed_intervals",
        )
        update = ",".join(f"{name}={name}+VALUES({name})" for name in update_columns)
        update += ",quality_flags=quality_flags|VALUES(quality_flags)"
        update += ",input_watermark=GREATEST(input_watermark,VALUES(input_watermark)),provisional=1"
        self._execute(
            "INSERT INTO telemetry_player_day ("
            + ",".join(PLAYER_DAY_COLUMNS)
            + ") VALUES (" + ",".join(["%s"] * len(PLAYER_DAY_COLUMNS)) + ") "
            "ON DUPLICATE KEY UPDATE "
            + update,
            tuple(values[name] for name in PLAYER_DAY_COLUMNS),
        )

    def _write_cohort(self, delta: Any) -> None:
        values: dict[str, Any] = {
            "definition_version": delta.target.definition_version,
            "generation": delta.target.generation,
            "environment_id": delta.target.environment_id,
            "season_id": delta.target.season_id,
            "utc_day": delta.utc_day,
            "level_band": delta.level_band,
            "class_id": delta.class_id,
            "race_id": delta.race_id,
            "faction_id": delta.faction_id,
            "zone_vnum": delta.zone_vnum,
            "config_id": delta.config_id,
            "category": delta.category,
            "duration_usec": delta.duration_usec,
            "attributable_usec": delta.attributable_usec,
            "observed_intervals": delta.observed_intervals,
            "subject_count": 0,
            "session_count": 0,
            "quality_flags": delta.quality_flags,
            "input_watermark": delta.input_watermark,
            "provisional": 1,
        }
        update = (
            "duration_usec=duration_usec+VALUES(duration_usec),"
            "attributable_usec=attributable_usec+VALUES(attributable_usec),"
            "observed_intervals=observed_intervals+VALUES(observed_intervals),"
            "quality_flags=quality_flags|VALUES(quality_flags),"
            "input_watermark=GREATEST(input_watermark,VALUES(input_watermark)),provisional=1"
        )
        self._execute(
            "INSERT INTO telemetry_cohort_day ("
            + ",".join(COHORT_COLUMNS)
            + ") VALUES (" + ",".join(["%s"] * len(COHORT_COLUMNS)) + ") "
            "ON DUPLICATE KEY UPDATE "
            + update,
            tuple(values[name] for name in COHORT_COLUMNS),
        )

    def _member_exists(self, delta: Any) -> bool:
        rows, _count, _ = self._execute(
            "SELECT 1 AS present FROM telemetry_cohort_member WHERE " + MEMBER_KEY_WHERE + " FOR UPDATE",
            (
                *delta.target.scope_tuple,
                delta.utc_day,
                delta.level_band,
                delta.class_id,
                delta.race_id,
                delta.faction_id,
                delta.zone_vnum,
                delta.config_id,
                delta.category,
                delta.membership_kind,
                delta.subject_id,
                delta.session_boot_id,
                delta.session_process_id,
                delta.session_seq,
            ),
        )
        return bool(rows)

    def _write_member(self, delta: Any) -> bool:
        existed = self._member_exists(delta)
        values: dict[str, Any] = {
            "definition_version": delta.target.definition_version,
            "generation": delta.target.generation,
            "environment_id": delta.target.environment_id,
            "season_id": delta.target.season_id,
            "utc_day": delta.utc_day,
            "level_band": delta.level_band,
            "class_id": delta.class_id,
            "race_id": delta.race_id,
            "faction_id": delta.faction_id,
            "zone_vnum": delta.zone_vnum,
            "config_id": delta.config_id,
            "category": delta.category,
            "membership_kind": delta.membership_kind,
            "subject_id": delta.subject_id,
            "session_boot_id": delta.session_boot_id,
            "session_process_id": delta.session_process_id,
            "session_seq": delta.session_seq,
            "duration_usec": delta.duration_usec,
            "attributable_usec": delta.attributable_usec,
            "observed_intervals": delta.observed_intervals,
            "quality_flags": delta.quality_flags,
            "input_watermark": delta.input_watermark,
        }
        update = (
            "duration_usec=duration_usec+VALUES(duration_usec),"
            "attributable_usec=attributable_usec+VALUES(attributable_usec),"
            "observed_intervals=observed_intervals+VALUES(observed_intervals),"
            "quality_flags=quality_flags|VALUES(quality_flags),"
            "input_watermark=GREATEST(input_watermark,VALUES(input_watermark))"
        )
        self._execute(
            "INSERT INTO telemetry_cohort_member ("
            + ",".join(MEMBER_COLUMNS)
            + ") VALUES (" + ",".join(["%s"] * len(MEMBER_COLUMNS)) + ") "
            "ON DUPLICATE KEY UPDATE "
            + update,
            tuple(values[name] for name in MEMBER_COLUMNS),
        )
        return existed

    def _increment_cohort_count(self, delta: Any) -> None:
        subject_increment = 1 if delta.membership_kind == 1 else 0
        session_increment = 1 if delta.membership_kind == 2 else 0
        _rows, affected, _ = self._execute(
            "UPDATE telemetry_cohort_day SET subject_count=subject_count+%s, "
            "session_count=session_count+%s WHERE " + COHORT_KEY_WHERE,
            (
                subject_increment,
                session_increment,
                *delta.target.scope_tuple,
                delta.utc_day,
                delta.level_band,
                delta.class_id,
                delta.race_id,
                delta.faction_id,
                delta.zone_vnum,
                delta.config_id,
                delta.category,
            ),
        )
        if affected == 0:
            raise DatabaseAccessError("cohort member had no aggregate row to count")

    def _update_state(self, target: RollupTarget, state: Mapping[str, Any], contribution: PageContribution) -> None:
        old_start = state.get("coverage_start_utc_usec")
        old_end = state.get("coverage_end_utc_usec")
        starts = [value for value in (old_start, contribution.coverage_start_utc_usec) if value is not None]
        ends = [value for value in (old_end, contribution.coverage_end_utc_usec) if value is not None]
        new_start = min(starts) if starts else None
        new_end = max(ends) if ends else None
        new_quality = int(state["quality_flags"]) | contribution.state_quality_flags
        self._execute(
            "UPDATE telemetry_rollup_state SET input_watermark=%s, "
            "coverage_start_utc_usec=%s, coverage_end_utc_usec=%s, "
            "quality_flags=%s, provisional=1 WHERE " + SCOPE_WHERE,
            (
                contribution.page_last_ingest_id,
                new_start,
                new_end,
                new_quality,
                *target.scope_tuple,
            ),
        )

    def _apply_contribution(
        self,
        target: RollupTarget,
        state: Mapping[str, Any],
        contribution: PageContribution,
    ) -> None:
        if contribution.start_cursor != int(state["input_watermark"]):
            raise CursorError(
                "page contribution was built from a cursor different from the locked state cursor"
            )
        if contribution.page_last_ingest_id <= contribution.start_cursor:
            raise CursorError("page contribution does not advance the input cursor")
        for key in sorted(contribution.sessions):
            delta = contribution.sessions[key]
            existing = self._read_locked_session(target, key)
            projected = merge_session_projection(existing, delta)
            self._write_session(projected)
        for key in sorted(contribution.player_days, key=repr):
            self._write_player_day(contribution.player_days[key])
        for key in sorted(contribution.cohorts, key=repr):
            self._write_cohort(contribution.cohorts[key])
        for key in sorted(contribution.members, key=repr):
            delta = contribution.members[key]
            existed = self._write_member(delta)
            if not existed:
                self._increment_cohort_count(delta)
        self._update_state(target, state, contribution)

    def _materialize_page(
        self,
        rows: Sequence[Mapping[str, Any]],
        target: RollupTarget,
        start_cursor: int,
        bounds: RollupBounds,
        max_bytes_remaining: int,
    ) -> _PageMaterial:
        contribution = build_page_contributions(
            rows,
            target,
            start_cursor=start_cursor,
            max_page_bytes=min(bounds.max_page_bytes, max_bytes_remaining),
            max_output_fanout=bounds.max_output_fanout,
        )
        if contribution.estimated_bytes > max_bytes_remaining:
            raise BoundsExceeded(
                f"raw page exceeds remaining invocation byte budget={max_bytes_remaining}"
            )
        return _PageMaterial(
            contribution=contribution,
            start_cursor=start_cursor,
            page_last_ingest_id=contribution.page_last_ingest_id,
            rows_fetched=len(rows),
            estimated_bytes=contribution.estimated_bytes,
        )

    def _reread_cursor_after_ambiguous(
        self,
        target: RollupTarget,
        bounds: RollupBounds,
        through_ingest_id: int,
        origin_ingest_id: int,
    ) -> int:
        """Reconcile on a new socket before any retry or later page."""

        self._ensure_connection()
        self._prepare_transaction_budget(bounds)
        acquired = False
        try:
            self._acquire_advisory_lock(target, bounds.lock_timeout_s)
            acquired = True
            self._begin()
            state = self._ensure_state_locked(target, through_ingest_id, origin_ingest_id)
            cursor = int(state["input_watermark"])
            self._rollback()
            return cursor
        finally:
            if acquired:
                self._release_advisory_lock()
            self._clear_transaction_budget()

    def process_next_page(
        self,
        target: RollupTarget,
        through_ingest_id: int,
        bounds: RollupBounds,
        build_page: Callable[
            [Sequence[Mapping[str, Any]], RollupTarget, int, int | None], PageContribution
        ],
        max_rows_remaining: int,
        max_bytes_remaining: int,
        origin_ingest_id: int = 0,
    ) -> RollupPageResult:
        """Lock state, fetch a page from that locked cursor, then commit atomically."""

        target.__post_init__()
        bounds.validate()
        _validate_cursor_value(through_ingest_id, "through_ingest_id")
        _validate_cursor_value(origin_ingest_id, "origin_ingest_id")
        if isinstance(max_rows_remaining, bool) or not isinstance(max_rows_remaining, int):
            raise ValueError("max_rows_remaining must be an integer")
        if isinstance(max_bytes_remaining, bool) or not isinstance(max_bytes_remaining, int):
            raise ValueError("max_bytes_remaining must be an integer")
        if max_rows_remaining <= 0 or max_bytes_remaining <= 0:
            raise BoundsExceeded("no invocation budget remains for a raw page")
        if through_ingest_id < origin_ingest_id:
            raise ValueError("through_ingest_id must be at least origin_ingest_id")
        material: _PageMaterial | None = None
        expected_start: int | None = None
        last_error: Exception | None = None

        retries = min(self.max_commit_retries, bounds.max_retries)
        fixed_deadline = self.clock() + float(bounds.max_runtime_s)
        for attempt in range(retries + 1):
            acquired = False
            try:
                self._active_page_deadline = fixed_deadline
                self._ensure_connection()
                self._prepare_transaction_budget(bounds)
                self._acquire_advisory_lock(target, bounds.lock_timeout_s)
                acquired = True
                self._begin()
                state = self._ensure_state_locked(target, through_ingest_id, origin_ingest_id)
                cursor = int(state["input_watermark"])
                if material is None:
                    expected_start = cursor
                    if cursor >= through_ingest_id:
                        # A newly bootstrapped empty generation must survive
                        # this acknowledgement so publication can race safely
                        # with another empty-scope worker.
                        self._commit()
                        return RollupPageResult(
                            status="complete",
                            cursor=cursor,
                            start_cursor=cursor,
                            quality_flags=int(state["quality_flags"]),
                        )
                    # Every selected C field is a fixed-width integer/NULL or
                    # 32-byte fingerprint. Reserve worst-case bytes BEFORE fetch,
                    # not after buffering an oversized page in the SQL driver.
                    row_byte_bound = 32 + sum(len(name.encode("utf-8")) + 8 + 32 for name in RAW_COLUMNS)
                    limit = min(bounds.page_size, max_rows_remaining,
                                min(bounds.max_page_bytes, max_bytes_remaining) // row_byte_bound)
                    if limit < 1:
                        raise BoundsExceeded("byte budget cannot reserve one bounded raw fact")
                    rows = self._fetch_raw_page(cursor, through_ingest_id, limit)
                    if not rows:
                        raise CursorError(
                            "raw keyset returned an empty page before the fixed high-water mark"
                        )
                    # The callback is supplied by the engine so fakes and the
                    # real adapter share exactly one pure semantic implementation.
                    built = build_page(
                        rows,
                        target,
                        cursor,
                        state.get("coverage_end_utc_usec"),
                    )
                    if built.start_cursor != cursor:
                        raise CursorError("page builder returned a stale cursor")
                    if built.fetched_rows != len(rows):
                        raise CursorError("page builder changed fetched row count")
                    if built.output_fanout > bounds.max_output_fanout:
                        raise BoundsExceeded("page builder exceeded output fanout bound")
                    if built.estimated_bytes < 0:
                        raise BoundsExceeded("page builder returned a negative byte estimate")
                    if built.page_last_ingest_id <= cursor or built.page_last_ingest_id > through_ingest_id:
                        raise CursorError("page builder returned a cursor outside the fixed keyset bound")
                    if built.estimated_bytes > min(bounds.max_page_bytes, max_bytes_remaining):
                        raise BoundsExceeded("page exceeds its explicit byte budget")
                    if len(rows) > max_rows_remaining:
                        raise BoundsExceeded("page exceeds remaining row budget")
                    material = _PageMaterial(
                        contribution=built,
                        start_cursor=cursor,
                        page_last_ingest_id=built.page_last_ingest_id,
                        rows_fetched=len(rows),
                        estimated_bytes=built.estimated_bytes,
                    )
                else:
                    if expected_start is None:
                        raise CursorError("ambiguous page lost its original cursor")
                    if cursor >= material.page_last_ingest_id:
                        self._rollback()
                        return RollupPageResult(
                            status="committed",
                            cursor=cursor,
                            start_cursor=material.start_cursor,
                            page_last_ingest_id=material.page_last_ingest_id,
                            fetched_rows=material.rows_fetched,
                            estimated_bytes=material.estimated_bytes,
                            output_fanout=material.contribution.output_fanout,
                            quality_flags=material.contribution.state_quality_flags,
                        )
                    if cursor != expected_start:
                        raise AmbiguousCommit(
                            "reread cursor is neither the original page start nor its end"
                        )
                assert material is not None
                self._apply_contribution(target, state, material.contribution)
                self._commit()
                return RollupPageResult(
                    status="committed",
                    cursor=material.page_last_ingest_id,
                    start_cursor=material.start_cursor,
                    page_last_ingest_id=material.page_last_ingest_id,
                    fetched_rows=material.rows_fetched,
                    estimated_bytes=material.estimated_bytes,
                    output_fanout=material.contribution.output_fanout,
                    quality_flags=material.contribution.state_quality_flags,
                )
            except AmbiguousCommit as error:
                last_error = error
                self._drop_connection()
                if material is None:
                    if expected_start is None:
                        raise
                    try:
                        cursor = self._reread_cursor_after_ambiguous(
                            target,
                            bounds,
                            through_ingest_id,
                            origin_ingest_id,
                        )
                    except Exception as reread_error:
                        self._drop_connection()
                        last_error = reread_error
                        if attempt >= retries:
                            raise AmbiguousCommit("empty-generation commit could not be reconciled") from reread_error
                        continue
                    if cursor >= through_ingest_id:
                        return RollupPageResult(
                            status="complete",
                            cursor=cursor,
                            start_cursor=cursor,
                            quality_flags=0,
                        )
                    # The bootstrap was rolled back.  The next bounded
                    # attempt recreates state and fetches the first page.
                    continue
                if expected_start is None:
                    raise
                try:
                    cursor = self._reread_cursor_after_ambiguous(
                        target,
                        bounds,
                        through_ingest_id,
                        origin_ingest_id,
                    )
                except Exception as reread_error:
                    self._drop_connection()
                    last_error = reread_error
                    if attempt >= retries:
                        raise AmbiguousCommit("ambiguous page commit could not be reconciled") from reread_error
                    continue
                if cursor >= material.page_last_ingest_id:
                    return RollupPageResult(
                        status="committed",
                        cursor=cursor,
                        start_cursor=material.start_cursor,
                        page_last_ingest_id=material.page_last_ingest_id,
                        fetched_rows=material.rows_fetched,
                        estimated_bytes=material.estimated_bytes,
                        output_fanout=material.contribution.output_fanout,
                        quality_flags=material.contribution.state_quality_flags,
                    )
                if cursor != expected_start:
                    raise AmbiguousCommit(
                        "reread cursor moved to an unrecognized position"
                    ) from error
                continue
            except Exception as error:
                self._rollback()
                if isinstance(error, BoundsExceeded) or isinstance(error, DatabaseAccessError) or not isinstance(error, RollupError):
                    self._drop_connection()
                raise
            finally:
                if acquired:
                    self._release_advisory_lock()
                self._clear_transaction_budget()
                self._active_page_deadline = None
        raise AmbiguousCommit("rollup page commit remained ambiguous after bounded retries") from last_error

    def _published_generations_locked(self, target: RollupTarget) -> list[int]:
        rows, _count, _ = self._execute(
            "SELECT generation,environment_id,season_id,publication_status "
            "FROM telemetry_rollup_state FORCE INDEX(PRIMARY) WHERE definition_version=%s "
            "ORDER BY generation,environment_id,season_id LIMIT %s FOR UPDATE",
            (target.definition_version, PUBLICATION_STATE_LIMIT + 1),
        )
        if len(rows) > PUBLICATION_STATE_LIMIT:
            raise BoundsExceeded("publication state capacity exceeded; no generation switched")
        published = [int(row["generation"]) for row in rows
                     if int(row["environment_id"]) == target.environment_id
                     and int(row["season_id"]) == target.season_id
                     and int(row["publication_status"]) == PUBLICATION_PUBLISHED]
        if len(published) > 1:
            raise GenerationConflict("multiple published generations require explicit reconciliation")
        return published

    @staticmethod
    def _publication_result(target: RollupTarget) -> Mapping[str, Any]:
        return {
            "status": "published",
            "definition_version": target.definition_version,
            "generation": target.generation,
            "environment_id": target.environment_id,
            "season_id": target.season_id,
        }

    def _publication_bounds(self, requested: RollupBounds | None) -> RollupBounds:
        # The state scan remains fixed and bounded; callers control only the
        # invocation deadline and retry ceiling through the validated policy.
        requested = requested or RollupBounds(max_runtime_s=10.0, max_retries=2)
        requested.validate()
        return RollupBounds(
            page_size=1,
            max_rows=PUBLICATION_STATE_LIMIT + 1,
            max_page_bytes=1024 * 1024,
            max_total_bytes=1024 * 1024,
            max_output_fanout=1,
            max_transaction_statements=100,
            max_runtime_s=requested.max_runtime_s,
            max_retries=requested.max_retries,
            statement_timeout_s=requested.statement_timeout_s,
            socket_timeout_s=requested.socket_timeout_s,
            lock_timeout_s=requested.lock_timeout_s,
        )

    def publish_generation(
        self,
        target: RollupTarget,
        *,
        bounds: RollupBounds | None = None,
    ) -> Mapping[str, Any]:
        """Publish one generation within one fixed retry/deadline budget."""

        target.__post_init__()
        publication_bounds = self._publication_bounds(bounds)
        retries = min(self.max_commit_retries, publication_bounds.max_retries)
        fixed_deadline = self.clock() + float(publication_bounds.max_runtime_s)
        self._active_page_deadline = fixed_deadline
        try:
            for attempt in range(retries + 1):
                acquired = False
                try:
                    self._deadline = fixed_deadline
                    self._check_deadline()
                    self._ensure_connection()
                    self._check_deadline()
                    self._prepare_transaction_budget(publication_bounds)
                    self._acquire_advisory_lock(target, publication_bounds.lock_timeout_s)
                    acquired = True
                    self._begin()
                    existing_state = self._fetch_state(target, for_update=True)
                    if existing_state is None and self.snapshot_high_watermark() != 0:
                        raise GenerationConflict(
                            "cannot publish an unprocessed generation while raw telemetry exists"
                        )
                    state = self._ensure_state_locked(target, 0, None)
                    if int(state["input_watermark"]) < int(state["rebuild_through_ingest_id"]):
                        raise GenerationConflict("cannot publish before the fixed input bound is processed")
                    status = int(state["publication_status"])
                    if status not in {PUBLICATION_BUILDING, PUBLICATION_PUBLISHED}:
                        raise GenerationConflict(f"generation status {status} is not publishable")
                    published = self._published_generations_locked(target)
                    newer = [generation for generation in published if generation > target.generation]
                    if newer:
                        raise GenerationConflict(
                            f"generation {target.generation} cannot supersede newer published generation(s) {newer}"
                        )
                    self._execute(
                        "UPDATE telemetry_rollup_state SET publication_status=%s, provisional=1 WHERE "
                        + SCOPE_WHERE,
                        (PUBLICATION_PUBLISHED, *target.scope_tuple),
                    )
                    for prior_generation in published:
                        if prior_generation != target.generation:
                            self._execute(
                                "UPDATE telemetry_rollup_state SET publication_status=%s WHERE "
                                + SCOPE_WHERE + " AND publication_status=%s",
                                (PUBLICATION_SUPERSEDED, target.definition_version,
                                 prior_generation, target.environment_id, target.season_id,
                                 PUBLICATION_PUBLISHED),
                            )
                    self._commit()
                    return self._publication_result(target)
                except AmbiguousCommit as error:
                    self._drop_connection()
                    if self.clock() >= fixed_deadline:
                        raise AmbiguousCommit(
                            "generation publication deadline expired during reconciliation"
                        ) from error
                    try:
                        self._deadline = fixed_deadline
                        self._ensure_connection()
                        self._check_deadline()
                        self._prepare_transaction_budget(publication_bounds)
                        row = self._fetch_state(target, for_update=False)
                        self._check_deadline()
                        if row is not None and int(row["publication_status"]) == PUBLICATION_PUBLISHED:
                            return self._publication_result(target)
                    except Exception as reread_error:
                        self._drop_connection()
                        if self.clock() >= fixed_deadline:
                            raise AmbiguousCommit(
                                "generation publication deadline expired during reconciliation"
                            ) from reread_error
                    if attempt >= retries:
                        raise AmbiguousCommit("generation publication commit remained ambiguous") from error
                    continue
                except Exception as error:
                    self._rollback()
                    if isinstance(error, BoundsExceeded) or isinstance(error, DatabaseAccessError) or not isinstance(error, RollupError):
                        self._drop_connection()
                    raise
                finally:
                    if acquired:
                        self._release_advisory_lock()
                    self._clear_transaction_budget()
        finally:
            self._active_page_deadline = None
        raise AmbiguousCommit("generation publication failed after bounded retries")

    def read_state(
        self,
        target: RollupTarget,
        *,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
        max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S,
    ) -> Mapping[str, Any] | None:
        target.__post_init__()
        self._prepare_report_budget(max_runtime_s, max_bytes, reserve_sentinel=False)
        try:
            row = self._fetch_state(target, for_update=False)
            if row is not None and _report_rows_bytes((row,)) > max_bytes:
                raise BoundsExceeded("rollup state row exceeds its report byte budget")
            self._check_deadline()
            return row
        except BoundsExceeded:
            self._drop_connection()
            raise
        finally:
            self._clear_transaction_budget()

    def _read_report_rows(
        self,
        target: RollupTarget,
        report_name: str,
        *,
        max_rows: int = REPORT_ROW_LIMIT_DEFAULT,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
    ) -> tuple[list[Mapping[str, Any]], bool]:
        definition = report_definition(report_name)
        table_and_columns = REPORT_TABLES.get(definition.name)
        if table_and_columns is None:
            raise ValueError(f"report has no executable aggregate query: {definition.name}")
        limit, fetch_limit = _report_fetch_limits(max_rows, max_bytes)
        table, columns = table_and_columns
        order_by = REPORT_ORDER_BY[definition.name]
        rows, _count, _ = self._execute(
            "SELECT " + ",".join(columns) + " FROM " + table
            + " WHERE " + SCOPE_WHERE
            + " ORDER BY " + order_by + " LIMIT %s",
            (*target.scope_tuple, fetch_limit),
        )
        if len(rows) > fetch_limit:
            raise BoundsExceeded("report adapter returned more rows than its SQL limit")
        if _report_rows_bytes(rows) > max_bytes:
            raise BoundsExceeded("report rows exceed their explicit byte budget")
        truncated = len(rows) > limit
        return rows[:limit], truncated

    @staticmethod
    def _public_report_rows(
        report_name: str,
        rows: Sequence[Mapping[str, Any]],
    ) -> tuple[Mapping[str, Any], ...]:
        """Translate storage sentinels into the stable #269 report interface.

        Support tables retain non-null indexed keys and checkpoint storage
        defaults.  Public rows must not make those defaults look observed:
        absent checkpoints are null and DATE 1000-01-01 is an explicit unknown
        bucket.
        """
        output: list[Mapping[str, Any]] = []
        for source in rows:
            row = dict(source)
            if report_name == "session_playtime":
                available = int(row.get("latest_checkpoint_revision", 0)) > 0
                row["checkpoint_totals_available"] = available
                if not available:
                    for name in COUNTER_FIELDS:
                        row[name] = None
            elif report_name == "cohort_activity":
                if row.get("utc_day") == UNKNOWN_DAY:
                    row["utc_day"] = None
                    row["bucket_kind"] = "unknown"
                else:
                    row["bucket_kind"] = "calendar"
            output.append(row)
        return tuple(output)

    def read_report(
        self,
        target: RollupTarget,
        report_name: str,
        *,
        max_rows: int = REPORT_ROW_LIMIT_DEFAULT,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
        max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S,
    ) -> ReportSnapshot:
        target.__post_init__()
        definition = report_definition(report_name)
        _report_fetch_limits(max_rows, max_bytes)
        self._prepare_report_budget(max_runtime_s, max_bytes, reserve_sentinel=True)
        # The writer uses READ COMMITTED, but report metadata and contributions
        # must belong to one snapshot even while a published generation advances.
        try:
            self._rollback()
            self._execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
            self._execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
            state = self._fetch_state(target, for_update=False)
            if state is None or int(state["publication_status"]) != PUBLICATION_PUBLISHED:
                raise GenerationConflict("requested report generation is not published")
            rows, truncated = self._read_report_rows(
                target,
                definition.name,
                max_rows=max_rows,
                max_bytes=max_bytes,
            )
            public_rows = self._public_report_rows(definition.name, rows)
            if _report_rows_bytes(public_rows) > max_bytes:
                raise BoundsExceeded("public report rows exceed their explicit byte budget")
            coverage = coverage_from_state_row(target, state)
            self._check_deadline()
            return ReportSnapshot(definition=definition, coverage=coverage, rows=public_rows, truncated=truncated)
        except BoundsExceeded:
            self._drop_connection()
            raise
        finally:
            self._rollback()
            self._clear_transaction_budget()


    def read_coverage(
        self,
        target: RollupTarget,
        *,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
        max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S,
    ) -> RollupCoverage:
        target.__post_init__()
        state = self.read_state(target, max_bytes=max_bytes, max_runtime_s=max_runtime_s)
        if state is None:
            raise DatabaseAccessError("requested generation has no rollup state")
        return coverage_from_state_row(target, state)

    def read_checkpoint_contributions(
        self,
        target: RollupTarget,
        *,
        max_rows: int = REPORT_ROW_LIMIT_DEFAULT,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
        max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S,
    ) -> tuple[CheckpointContribution, ...]:
        target.__post_init__()
        limit, fetch_limit = _report_fetch_limits(max_rows, max_bytes)
        self._prepare_report_budget(max_runtime_s, max_bytes, reserve_sentinel=True)
        try:
            rows, _count, _ = self._execute(
                "SELECT " + ",".join(SESSION_COLUMNS)
                + " FROM telemetry_rollup_session WHERE " + SCOPE_WHERE
                + " ORDER BY subject_id,session_boot_id,session_process_id,session_seq LIMIT %s",
                (*target.scope_tuple, fetch_limit),
            )
            if len(rows) > fetch_limit or _report_rows_bytes(rows) > max_bytes:
                raise BoundsExceeded("checkpoint contributions exceed their explicit read budget")
            if len(rows) > limit:
                raise BoundsExceeded("checkpoint contributions exceed max_rows; no complete distribution returned")
            return tuple(checkpoint_contribution_from_row(target, row) for row in rows)
        except BoundsExceeded:
            self._drop_connection()
            raise
        finally:
            self._clear_transaction_budget()

    def read_membership_contributions(
        self,
        target: RollupTarget,
        *,
        membership_kind: int | None = None,
        max_rows: int = REPORT_ROW_LIMIT_DEFAULT,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
        max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S,
    ) -> tuple[MembershipContribution, ...]:
        target.__post_init__()
        limit, fetch_limit = _report_fetch_limits(max_rows, max_bytes)
        statement = (
            "SELECT " + ",".join(MEMBER_COLUMNS)
            + " FROM telemetry_cohort_member WHERE " + SCOPE_WHERE
        )
        parameters: tuple[Any, ...] = target.scope_tuple
        if membership_kind is not None:
            if membership_kind not in (1, 2):
                raise ValueError("membership_kind must be 1 (subject) or 2 (session)")
            statement += " AND membership_kind=%s"
            parameters += (membership_kind,)
        statement += " ORDER BY utc_day,level_band,class_id,race_id,faction_id,zone_vnum,config_id,category,subject_id,session_boot_id,session_process_id,session_seq LIMIT %s"
        parameters += (fetch_limit,)
        self._prepare_report_budget(max_runtime_s, max_bytes, reserve_sentinel=True)
        try:
            rows, _count, _ = self._execute(statement, parameters)
            if len(rows) > fetch_limit or _report_rows_bytes(rows) > max_bytes:
                raise BoundsExceeded("membership contributions exceed their explicit read budget")
            if len(rows) > limit:
                raise BoundsExceeded("membership contributions exceed max_rows; no complete distribution returned")
            return tuple(membership_contribution_from_row(target, row) for row in rows)
        except BoundsExceeded:
            self._drop_connection()
            raise
        finally:
            self._clear_transaction_budget()


# Names used by callers/tests that prefer a shorter adapter spelling.
RollupDatabaseAdapter = PyMySQLRollupDatabase
RestrictedRollupDatabase = PyMySQLRollupDatabase


__all__ = [
    "AmbiguousCommit",
    "BoundsExceeded",
    "ConnectionSettings",
    "ConcurrentConnectionError",
    "DatabaseAccessError",
    "GenerationConflict",
    "LockTimeout",
    "MEMBER_COLUMNS",
    "PLAYER_DAY_COLUMNS",
    "PyMySQLConnectionFactory",
    "PyMySQLRollupDatabase",
    "RAW_COLUMNS",
    "REPORT_BYTE_LIMIT_DEFAULT",
    "REPORT_BYTE_LIMIT_HARD_MAX",
    "REPORT_ROW_BYTE_BOUND",
    "REPORT_ROW_LIMIT_DEFAULT",
    "REPORT_ROW_LIMIT_HARD_MAX",
    "REPORT_RUNTIME_DEFAULT_S",
    "REPORT_TABLES",
    "RollupDatabaseAdapter",
    "RestrictedRollupDatabase",
    "SESSION_COLUMNS",
    "STATE_COLUMNS",
]
