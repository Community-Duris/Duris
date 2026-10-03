#!/usr/bin/env python3
"""S03 necromancy SQL compatibility regression journeys.

The fixture compiles the exact owned post-commit C++-compiled C function bodies
against an isolated in-memory world. A separate disposable MariaDB repository
journey checks durable corpse item UID/root/parent and custody state. It does not
activate the SQL lifecycle in a full game server.
"""
from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from pa_accounting_batch_artifact import load_base_build, report_base_build
from pa_necromancy_fixture import ROOT, run_runtime_fixture, _function

SQL_RUNNER = ROOT / "tests/async/run_corpse_lifecycle_repository_schema_mysql.sh"
HARNESS = ROOT / "tests/async/corpse_lifecycle_repository_mysql_harness.cpp"
LIFECYCLE_SOURCE = ROOT / "src/magic/spell_corpse_lifecycle.c"

EXTENDED_SQL_HARNESS = r'''#define main corpse_lifecycle_original_main
#include "../corpse_lifecycle_repository_mysql_harness.cpp"
#include <iostream>
#undef main

static void reconnect_sql_session()
{
    mysql_close(database);
    database = mysql_init(nullptr);
    assert(database);
    assert(mysql_real_connect(database, getenv("DB_HOST"), getenv("DB_USER"),
        getenv("DB_PASSWD"), getenv("CORPSE_LIFECYCLE_TEST_DB_NAME"),
        static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
}

int main()
{
    database = mysql_init(nullptr);
    assert(database);
    assert(mysql_real_connect(database, getenv("DB_HOST"), getenv("DB_USER"),
        getenv("DB_PASSWD"), getenv("CORPSE_LIFECYCLE_TEST_DB_NAME"),
        static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr, 0));
    execute("UPDATE collector_catalog_state SET next_listing=100000 WHERE state_id=1");

    test_raise_follower();
    reconnect_sql_session();
    assert(text("SELECT CONCAT(r.obj_uid,':',c.obj_uid,':',o.root_item_uid,':',"
        "o.parent_item_uid,':',o.owner_type,':',o.owner_id) FROM player_items r "
        "JOIN player_items c ON c.container_id=r.id JOIN item_current_owner o "
        "ON o.item_uid=c.obj_uid WHERE r.pid=2147000603 AND r.obj_uid=840000001 "
        "AND c.obj_uid=840000002") ==
        "840000001:840000002:840000001:840000001:1:2147000603");
    std::cout << "SQL_RAISE root_uid=840000001 child_uid=840000002 "
        "parent_uid=840000001 owner_pid=2147000603 after_sql_reconnect=1\n";

    test_resurrection();
    reconnect_sql_session();
    assert(text("SELECT CONCAT(r.obj_uid,':',c.obj_uid,':',o.root_item_uid,':',"
        "o.parent_item_uid,':',o.owner_type,':',o.owner_id) FROM player_items r "
        "JOIN player_items c ON c.container_id=r.id JOIN item_current_owner o "
        "ON o.item_uid=c.obj_uid WHERE r.pid=2147000602 AND r.obj_uid=830000001 "
        "AND c.obj_uid=830000002") ==
        "830000001:830000002:830000001:830000001:1:2147000602");
    std::cout << "SQL_RESURRECTION root_uid=830000001 child_uid=830000002 "
        "parent_uid=830000001 owner_pid=2147000602 after_sql_reconnect=1\n";

    mysql_close(database);
    return 0;
}
'''


class NecromancyCorpseSqlJourneys(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.base_build = load_base_build(ROOT)
        report_base_build(cls.base_build, scope="S03 source/component fixtures; frozen binary not executed")

    def test_animate_and_resurrection_visible_contents_journeys(self) -> None:
        output = run_runtime_fixture()
        self.assertIn("RAISE visible_room=3 pet_uid=99001 owner_pid=42101", output)
        self.assertIn("root_uid=301 child_uid=302 parent_uid=301", output)
        self.assertIn("transient_destroyed=2 checkpoint_calls=1", output)
        self.assertIn("RESURRECTION visible_room=3 old_inventory_room=2", output)
        self.assertIn("root_uid=420 child_uid=421 parent_uid=420", output)
        self.assertIn("transient_destroyed=1 checkpoint_calls=1", output)
        print("S03 runtime fixture passed; production function bodies were compiled from source")

    def test_resurrection_spell_eligibility_and_binding_guards_remain(self) -> None:
        source = LIFECYCLE_SOURCE.read_text()
        for spell, defer_call in (
            ("spell_resurrect", "persistence_defer_corpse_resurrection(obj, ch, t_ch, false)"),
            ("spell_lesser_resurrect", "persistence_defer_corpse_resurrection(obj, ch, t_ch, true)"),
        ):
            body = _function(source, spell, "void")
            self.assertIn("!is_linked_to(ch, t_ch, LNK_CONSENT)", body, spell)
            self.assertIn("GET_PID(t_ch) != obj->value[3]", body, spell)
            self.assertIn("if ((obj->value[2] < 0) && !IS_TRUSTED(ch))", body, spell)
            self.assertIn("if (resurrection_item_is_transient(t_obj))", body, spell)
            self.assertIn(defer_call, body, spell)
            self.assertLess(body.index("GET_PID(t_ch) != obj->value[3]"), body.index(defer_call))
            self.assertLess(body.index("!is_linked_to(ch, t_ch, LNK_CONSENT)"),
                            body.index(defer_call))

    def test_durable_mysql_corpse_contents_and_custody_journeys(self) -> None:
        self.assertTrue(SQL_RUNNER.is_file())
        harness_source = HARNESS.read_text()
        self.assertIn("test_resurrection();", harness_source)
        self.assertIn("test_raise_follower();", harness_source)
        self.assertIn("root_item_uid", harness_source)
        self.assertIn("parent_item_uid", harness_source)
        self.assertIn("item_current_owner", harness_source)
        with tempfile.TemporaryDirectory(prefix=".pa-necromancy-sql-",
                                         dir=ROOT / "tests/async") as temporary:
            temp = Path(temporary)
            source = temp / "s03_corpse_sql_journeys.cpp"
            runner = temp / "run_s03_corpse_sql_journeys.sh"
            source.write_text(EXTENDED_SQL_HARNESS)
            script = SQL_RUNNER.read_text()
            root_assignment = 'ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"'
            self.assertIn(root_assignment, script)
            script = script.replace(root_assignment, f'ROOT="{ROOT}"', 1)
            script = script.replace("tests/async/corpse_lifecycle_repository_mysql_harness.cpp",
                                    str(source.relative_to(ROOT)), 1)
            script = script.replace(
                "corpse lifecycle authority, materialization, collector, currency, artifact, replay, and rollback transactions (%s): ok",
                "S03 raise and resurrection nested UID SQL journeys: ok", 1)
            runner.write_text(script)
            env = os.environ.copy()
            env["CORPSE_LIFECYCLE_REPOSITORY_DB_HOST"] = "127.0.0.1"
            before = _docker_containers()
            result = subprocess.run(["bash", str(runner)], cwd=ROOT, env=env,
                                    text=True, capture_output=True, check=False)
        after = _docker_containers()
        leaked = sorted(name for name in after - before if name.startswith(
            "duris-corpse-lifecycle-repository-"))
        self.assertFalse(leaked, "task-created disposable SQL containers remained after runner cleanup")
        self.assertEqual(result.returncode, 0,
                         f"disposable MariaDB corpse journey exit={result.returncode}; "
                         f"stdout={result.stdout[-3000:]} stderr={result.stderr[-3000:]}")
        self.assertIn("SQL_RAISE root_uid=840000001 child_uid=840000002", result.stdout)
        self.assertIn("parent_uid=840000001 owner_pid=2147000603 after_sql_reconnect=1", result.stdout)
        self.assertIn("SQL_RESURRECTION root_uid=830000001 child_uid=830000002", result.stdout)
        self.assertIn("parent_uid=830000001 owner_pid=2147000602 after_sql_reconnect=1", result.stdout)
        self.assertIn("S03 raise and resurrection nested UID SQL journeys: ok", result.stdout)
        print(result.stdout.strip())


def _docker_containers() -> set[str]:
    result = subprocess.run(["docker", "ps", "-a", "--format", "{{.Names}}"],
                            text=True, capture_output=True, check=False)
    if result.returncode:
        raise AssertionError("cannot verify disposable Docker fixture cleanup")
    return {name for name in result.stdout.splitlines() if name}


if __name__ == "__main__":
    unittest.main(verbosity=2)
