#!/usr/bin/env python3
"""Source-only guard for the disposable item-state schema prerequisite."""
import ast
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests/async/pa_item_flags_fixture.py"
source = FIXTURE.read_text()
runner = next(
    node for node in ast.parse(source).body
    if isinstance(node, ast.FunctionDef) and node.name == "run_item_flags_fixture"
)
migration_loop = next(
    node for node in ast.walk(runner)
    if isinstance(node, ast.For)
    and isinstance(node.target, ast.Name)
    and node.target.id == "migration"
)
assert isinstance(migration_loop.iter, ast.Tuple)

def root_path(node: ast.AST) -> str:
    assert (
        isinstance(node, ast.BinOp)
        and isinstance(node.op, ast.Div)
        and isinstance(node.left, ast.Name)
        and node.left.id == "ROOT"
        and isinstance(node.right, ast.Constant)
        and isinstance(node.right.value, str)
    )
    return node.right.value


migrations = tuple(root_path(node) for node in migration_loop.iter.elts)
assert migrations == (
    "migrations/bootstrap_multithread_safe.sql",
    "migrations/immutable/0015_output_preferences.sql",
    "migrations/immutable/0020_player_death_restitution.sql",
    "migrations/immutable/0035_player_item_dynamic_state.sql",
)
assert re.search(
    r'verifier = ROOT / "migrations/immutable/0035_player_item_dynamic_state.sh"'
    r'.*?"docker", "exec", "-i".*?"DB_HOST".*?"DB_NAME"'
    r'.*?"bash", "-s".*?input_text=verifier\.read_text\(\)',
    source,
    re.DOTALL,
)
print("item flags fixture schema prerequisite source contract passed")
