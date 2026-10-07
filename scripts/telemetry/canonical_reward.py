"""Bounded canonical reward capture/stage/publish/report with dedicated DB roles.

Run as ``python3 -m scripts.telemetry.canonical_reward``. Project commands use
TELEMETRY_REWARD_PROJECT_DB_*; capture uses TELEMETRY_REWARD_SOURCE_DB_*;
report uses TELEMETRY_REWARD_REPORT_DB_*. Passwords
come only from those environment namespaces, never arguments or game DB_*.
"""
from __future__ import annotations

import argparse
import json
import sys
import time

from . import canonical_reward_contract as contract
from .canonical_reward_publication import CanonicalRewardPublisher, CanonicalRewardReporter
from .canonical_reward_source import CanonicalRewardSnapshot, MAX_OPERATIONS
from .canonical_reward_coin_source import CanonicalCoinSnapshot
from .canonical_reward_auction_source import CanonicalAuctionSnapshot
from .canonical_reward_retention import CanonicalRewardRetention
from .db_access import ConnectionSettings, PyMySQLConnectionFactory
from .reward_projection import AmbiguousCommit, ProjectionBoundsExceeded
from .rollup import _run_killable_process
from .rollup_engine import BoundsExceeded


def _identifier(value, *, size):
    try:
        decoded = bytes.fromhex(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("use an exact hexadecimal identifier") from error
    if len(value) != size * 2 or len(decoded) != size or not any(decoded):
        raise argparse.ArgumentTypeError("use an exact nonzero hexadecimal identifier")
    return decoded


def _json(value):
    if isinstance(value, bytes):
        return value.hex()
    raise TypeError("unsupported reward report value")


def _worker(command, generation_id, cut_ids, scan_id, byte_limit, runtime, route=None, operations=(), quarantine=False):
    cut = None
    try:
        prefix = {"report": "TELEMETRY_REWARD_REPORT_DB_", "capture": "TELEMETRY_REWARD_SOURCE_DB_"}.get(
            command, "TELEMETRY_REWARD_PROJECT_DB_")
        factory = PyMySQLConnectionFactory(ConnectionSettings.from_env(prefix))
        options = dict(byte_limit=byte_limit, time_limit_s=runtime)
        if command == "capture":
            deadline = time.monotonic() + runtime
            adapter, method = {"bank": (CanonicalRewardSnapshot, "capture_bank_operations"),
                "wallet-pile": (CanonicalCoinSnapshot, "capture_coin_operations"),
                "auction": (CanonicalAuctionSnapshot, "capture_money_claims")}[route]
            cut = getattr(adapter(factory, **options), method)(operations, preserve_refusals=quarantine)
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise ProjectionBoundsExceeded("canonical capture and retention deadline")
            CanonicalRewardRetention(factory, byte_limit=byte_limit, time_limit_s=remaining).retain(cut)
            result = dict(cut_id=cut.source_digest, definition_version=contract.DEFINITION_VERSION,
                captured_utc_usec=cut.captured_utc_usec, selected_count=len(cut.selected_operations),
                source_rows=len(cut.sources), event_count=len(cut.events), source_queries=cut.query_counts,
                reserved_bytes=cut.reserved_bytes, coverage=cut.coverage, future_commits_provisional=True,
                unknown_count=sum(event.disposition in (contract.Disposition.UNKNOWN, contract.Disposition.CONFLICT)
                                  for event in cut.events))
        elif command == "report":
            result = CanonicalRewardReporter(factory, **options).read(generation_id)
        elif command == "stage":
            plan = CanonicalRewardPublisher(factory, **options).stage(generation_id, cut_ids, scan_id=scan_id)
            result = {name: plan.header[name] for name in ("generation_id", "definition_version", "snapshot_utc_usec",
                "cut_count", "event_count", "reserved_bytes", "evidence_digest", "projection_digest")}
            result["future_commits_provisional"] = True
        else:
            result = CanonicalRewardPublisher(factory, **options).publish(generation_id)
        print(json.dumps(result, default=_json, sort_keys=True), flush=True)
    except (contract.EvidenceError, ProjectionBoundsExceeded) as error:
        print(json.dumps(dict(status="refused", error=str(error))), file=sys.stderr, flush=True)
        raise SystemExit(1)
    except AmbiguousCommit:
        identity = dict(cut_id=cut.source_digest.hex()) if cut is not None else dict(generation_id=generation_id.hex())
        recovery = "verify the exact sealed cut identity before staging; recapture creates a new snapshot" if cut is not None else \
            "retry the same command and generation identity"
        print(json.dumps(dict(status="ambiguous_commit", **identity, recovery=recovery)), file=sys.stderr, flush=True)
        raise SystemExit(1)
    except Exception:
        # DB driver exceptions can contain private values; do not serialize them.
        print(json.dumps(dict(status="refused", error="reward_command_failed")), file=sys.stderr, flush=True)
        raise SystemExit(1)


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("capture", "stage", "publish", "report"))
    parser.add_argument("--generation", type=lambda value: _identifier(value, size=16))
    parser.add_argument("--route", choices=("bank", "wallet-pile", "auction"), help="native capture adapter")
    parser.add_argument("--operation", action="append", default=[], type=lambda value: _identifier(value, size=16),
                        help="exact native root operation; repeat for a bounded capture selection")
    parser.add_argument("--quarantine", action="store_true", help="retain bounded receipt refusals with unknown authority")
    parser.add_argument("--cut", action="append", default=[], type=lambda value: _identifier(value, size=32),
                        help="exact retained cut identity; repeat for a bounded stage selection")
    parser.add_argument("--scan", type=lambda value: _identifier(value, size=16),
                        help="optional exact sweep identity for coverage captured with a new generation")
    parser.add_argument("--byte-limit", type=int, default=32 * 1024 * 1024)
    parser.add_argument("--runtime", type=float, default=10.0, help="whole-command deadline, at most ten seconds")
    return parser


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)
    if not 4096 <= args.byte_limit <= 32 * 1024 * 1024 or not 0 < args.runtime <= 10:
        parser.error("byte limit must be 4096..33554432 and runtime must be positive and at most ten seconds")
    cuts = tuple(sorted(set(args.cut)))
    if args.command == "capture":
        if args.generation is not None or cuts or args.scan is not None or args.route is None or \
                not 0 < len(args.operation) <= MAX_OPERATIONS or len(set(args.operation)) != len(args.operation):
            parser.error("capture needs a route and 1..16384 distinct native operations, without generation/cut/scan")
    elif args.generation is None or args.route is not None or args.operation or args.quarantine:
        parser.error("stage/publish/report need a generation and do not accept capture options")
    if (args.command == "stage" and not 0 < len(cuts) <= 256) or (args.command != "stage" and cuts):
        parser.error("stage needs 1..256 exact cuts; publish/report use the reserved generation")
    if args.scan is not None and args.command != "stage":
        parser.error("scan is a stage selection; publish/report use its retained evidence")
    try:
        return _run_killable_process(_worker, (args.command, args.generation, cuts, args.scan, args.byte_limit,
            args.runtime, args.route, tuple(sorted(args.operation)), args.quarantine), timeout_s=args.runtime)
    except BoundsExceeded:
        identity = dict(generation_id=args.generation.hex()) if args.generation is not None else dict(route=args.route)
        recovery = "retry the same command and generation identity to reconcile a possibly committed write" if args.generation is not None else \
            "capture may have retained an unpublished snapshot; recapture cannot duplicate canonical earnings"
        print(json.dumps(dict(status="deadline", **identity, recovery=recovery)), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
