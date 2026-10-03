#!/usr/bin/env python3
"""Exercise auction inspector lifetime without compiling or starting a server."""

import os
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
from unittest.mock import Mock, patch

# Only the entrypoint orchestration is under test. Stand-ins keep this check
# independent of the Linux-only runtime helpers, compiler, and running server.
journey = SimpleNamespace(
    ROOT=Path(__file__).resolve().parents[2],
    INSPECTOR=Path("unused-shared-inspector"),
    make_fixture=Mock(), build_inspector=Mock(), build_flatfile_server=Mock(), run_journey=Mock(),
)
first_session = SimpleNamespace(verify_first_session=Mock())
with patch.dict(sys.modules, {
    "test_flatfile_combat_journey": journey,
    "test_flatfile_first_session_currency": first_session,
}):
    import test_flatfile_auction_coin_put_journey as auction


class InjectedFailure(RuntimeError):
    pass


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    original_inspector = auction.journey.INSPECTOR
    original_fixture = auction.journey.make_fixture
    original_verify = auction.first_session.verify_first_session
    destinations = []

    # Exercise a fresh bin/tests tree, repeated invocations in the same process,
    # supplied-server mode, and cleanup after each stage fails.
    for mode in ("cold", "server", "inspector-failure", "build-failure", "run-failure"):
        supplied = root / "supplied-server"
        calls = []

        def build_inspector(inspector):
            # Follow the journey's inspector builder; its compiler/deadline
            # contract is separate from this lifetime and isolation check.
            assert inspector.parent.parent == root / "bin/tests"
            assert inspector.parent.name.startswith(f"auction-coin-put-{os.getpid()}-")
            assert inspector not in destinations
            assert inspector != original_inspector
            destinations.append(inspector)
            inspector.write_text("private inspector")
            calls.append("inspector")
            if mode == "inspector-failure":
                raise InjectedFailure(mode)

        def build_server(build_root):
            assert build_root == destinations[-1].parent
            calls.append("build")
            if mode == "build-failure":
                raise InjectedFailure(mode)
            return build_root / "server"

        def run_journey(binary, **kwargs):
            inspector = destinations[-1]
            assert auction.journey.INSPECTOR == inspector
            assert inspector.read_text() == "private inspector"
            assert binary == (supplied if mode == "server" else inspector.parent / "server")
            assert kwargs == dict(reset_coins=True, first_session_only=True)
            assert auction.journey.make_fixture is auction.make_fixture
            # The callback must still propagate --expect-regression.
            with patch.object(auction, "verify") as verify:
                auction.first_session.verify_first_session("client", 1234, root, False)
                verify.assert_called_once_with("client", 1234, root, False, mode == "server")
            calls.append("run")
            if mode == "run-failure":
                raise InjectedFailure(mode)

        arguments = ["auction"]
        if mode == "server":
            arguments += ["--server", str(supplied), "--expect-regression"]
        with (patch.object(auction.journey, "ROOT", root),
              patch.object(sys, "argv", arguments),
              patch.object(auction.journey, "build_inspector", side_effect=build_inspector),
              patch.object(auction.journey, "build_flatfile_server", side_effect=build_server),
              patch.object(auction.journey, "run_journey", side_effect=run_journey)):
            try:
                auction.main()
            except InjectedFailure as error:
                assert str(error) == mode and mode.endswith("-failure")
            else:
                assert not mode.endswith("-failure"), "failure was swallowed"

        assert auction.journey.INSPECTOR == original_inspector
        assert auction.journey.make_fixture is original_fixture
        assert auction.first_session.verify_first_session is original_verify
        assert not destinations[-1].parent.exists(), "temporary build directory leaked"
        assert not (root / "bin/tests/coin-death-inspector").exists()
        expected = {
            "cold": ["inspector", "build", "run"],
            "server": ["inspector", "run"],
            "inspector-failure": ["inspector"],
            "build-failure": ["inspector", "build"],
            "run-failure": ["inspector", "build", "run"],
        }
        assert calls == expected[mode]

print("auction private inspector, supplied server, and failure cleanup passed")
