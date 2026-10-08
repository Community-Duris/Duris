"""SELECT-only room diagnostics for a quiescent restore or borrowed RR cut.

Canonical accounting roots remain the existing restore verifier's responsibility.
These checks do not authenticate all original payload bytes or runtime publication.
"""
from collections import Counter
import json

from economic_item_payload_audit import PayloadError, audit_room_items
from economic_sql_audit_snapshot import ExportError, read_room_item_custody
from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS

INTEGER_SOURCES = {
    "sql_room_item_payload": ("item_uid", "item_revision", "payload_version", "season_epoch"),
    "item_current_owner": ("item_uid", "root_item_uid", "parent_item_uid", "owner_type", "owner_id",
        "owner_context_id", "item_revision", "vnum", "state", "equipment_slot"),
    "item_owner_revision": ("owner_type", "owner_id", "owner_context_id", "revision"),
    "season_reset_state": ("state_id", "season_epoch"),
    "saved_items": ("obj_uid",),
    "economic_accounting_item_reference": ("event_index", "line_index", "child_index", "item_uid",
        "before_revision", "after_revision", "legacy_event_index"),
    "item_ownership_ledger": ("event_index", "item_uid", "root_item_uid", "parent_item_uid",
        "from_owner_type", "from_owner_id", "from_owner_context_id", "to_owner_type", "to_owner_id",
        "to_owner_context_id", "item_revision", "reason_type", "reason_id"),
    "critical_operation_inbox": ("command_type", "schema_version", "status", "result_code", "failure_stage"),
    "economic_accounting_operation": ("outcome", "result_code"),
}
UNQUALIFIED = {"room_item_full_runtime_authority_unqualified", "room_item_retained_root_authority_unqualified"}


def refuse(code):
    raise RuntimeError("restore_room_item_" + code)


class RoomRows:
    """Adapt the existing bounded SQL CLI to the room reader's second caller."""
    def __init__(self, executor):
        self.executor = executor
        self.bytes = self.rows = 0

    def __call__(self, query, columns, binary):
        expressions = ["IF(r." + name + " IS NULL,NULL,LOWER(HEX(r." + name + ")))" if name in binary
                       else "r." + name for name in columns]
        page_size = 4 if "payload" in binary else 256
        result, offset = [], 0
        while True:
            sql = "SELECT JSON_ARRAY(" + ",".join(expressions) + ") FROM (" + query + " LIMIT " + str(page_size) + \
                  " OFFSET " + str(offset) + ") r;"
            output = self.executor.sql(sql)
            if not isinstance(output, str):
                refuse("packet_invalid")
            self.bytes += len(output.encode("utf-8"))
            lines = output.splitlines()
            self.rows += len(lines)
            if self.bytes > MAX_INPUT_BYTES or self.rows > MAX_ROWS + 1 or len(lines) > page_size:
                refuse("capture_bounds")
            for line in lines:
                try:
                    values = json.loads(line)
                    if type(values) is not list or len(values) != len(columns):
                        refuse("packet_invalid")
                    row = dict(zip(columns, values))
                    for name in binary:
                        value = row[name]
                        if value is not None:
                            if not isinstance(value, str) or len(value) % 2 or any(c not in "0123456789abcdef" for c in value):
                                refuse("packet_invalid")
                            row[name] = bytes.fromhex(value)
                    result.append(row)
                except (ValueError, TypeError):
                    refuse("packet_invalid")
            if len(lines) < page_size:
                return result
            offset += page_size


def capture_room_items(executor):
    """Keep every raw row; pages are enumeration, never live commit watermarks."""
    tables = ",".join("'" + name + "'" for name in INTEGER_SOURCES)
    if executor.sql("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
                    "AND ENGINE='InnoDB' AND table_name IN (" + tables + ");") != str(len(INTEGER_SOURCES)):
        refuse("source_invalid")
    scopes = ["(table_name='" + table + "' AND column_name IN (" +
              ",".join("'" + name + "'" for name in names) + "))" for table,names in INTEGER_SOURCES.items()]
    value = executor.sql("SELECT JSON_ARRAY(COUNT(*),COALESCE(SUM(data_type NOT IN "
        "('tinyint','smallint','mediumint','int','bigint')),0)) FROM information_schema.columns "
        "WHERE table_schema=DATABASE() AND (" + " OR ".join(scopes) + ");")
    try:
        shape = json.loads(value)
    except (ValueError, TypeError):
        refuse("source_invalid")
    if (type(shape) is not list or len(shape) != 2 or any(type(n) is not int for n in shape) or
            shape != [sum(map(len, INTEGER_SOURCES.values())), 0]):
        refuse("source_invalid")
    try:
        native = read_room_item_custody(None, select_rows=RoomRows(executor))
        if len(json.dumps(native).encode("utf-8")) > MAX_INPUT_BYTES:
            refuse("capture_bounds")
        return native
    except (ExportError, PayloadError, TypeError, ValueError, KeyError):
        refuse("packet_invalid")


def require_room_item_integrity(executor):
    native = capture_room_items(executor)
    counts = Counter()
    try:
        audit_room_items(native, lambda code,**details: counts.update((code,)), MAX_ROWS, MAX_INPUT_BYTES)
    except PayloadError:
        refuse("packet_invalid")
    failures = sorted(set(counts) - UNQUALIFIED)
    if failures:
        refuse(failures[0].removeprefix("room_item_"))
    # Do not promote structural checks to original-payload or runtime authority.
    return dict(counts)
