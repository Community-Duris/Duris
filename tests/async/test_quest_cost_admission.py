#!/usr/bin/env python3
"""Run the real final Quest cost projection and temporary Item admission slice.

Preserves the reviewed admission template, ordinary whole Quest/Item builds,
15 real provider objects, SQL success, flat/assertions refusal and current
selector regression. Extracted Quest declarations/bodies come from source;
the fixture includes the whole Item implementation. Requires Linux GNU13.3/
libstdc++13 production flags. No SQL/Redis service, game or recovery run.
Earlier preparation, builder and full producer integration remain separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ORDINARY = ("world/quest", "item/item_movement_transaction")
PROVIDERS = (
    "economy/native_quest_cost", "persistence/critical_command",
    "persistence/critical_command_journal", "persistence/persistence_mode",
    "world/new_events", "core/common", "core/utility",
    "player/player_snapshot_codec", "world/quest_mobile_native_reference",
    "world/quest_mobile_native_binding", "world/native_quest_recovery_context",
    "world/quest_mobile_native", "item/item_transfer_command",
    "item/lockpick_retirement_continuation", "economy/native_quest_coin_give",
)


def generate(source, work, template):
    from _paths import extract_function
    from contract_text import code_text, contains, count, index
    quest = source / "src/world/quest.c"
    text = quest.read_text(encoding="utf-8")

    def body(signature):
        assert count(text, signature) == 1, signature
        return extract_function(str(quest), signature)

    alias_signature = "using quest_cost_prepare_reserve ="
    assert count(text, alias_signature) == 1
    alias_start = index(text, alias_signature)
    alias_end = code_text(text).index(";", alias_start) + 1
    descriptor = text[alias_start:alias_end] + "\n" + body("struct quest_cost_prepare_live") + ";"
    selected_descriptor = body("struct quest_selected_encode_live") + ";"
    preparation = body("item_native_quest_preparation_state quest_native_completion_owner::prepare_original(")
    assert count(preparation, "encode_selected_items(") == 1
    assert not contains(preparation, "player_item_snapshot_list_encode(selected, &selected_bytes)")
    checks = ("selected_roots != consumed.size()", "(!fee_only && selected.empty())",
              "selected.size() > ITEM_TRANSFER_MAX_ITEMS", "encode_selected_items(")
    positions = [index(preparation, check) for check in checks]
    assert positions == sorted(positions)
    assert contains(preparation, "encode_selected_items(&selected_bytes, quest_selected_encode_live{")
    current = body("bool quest_cost_prepare_current(")
    current_start = text.index(current)
    guard_start = code_text(text).rfind("#if", 0, current_start)
    assert guard_start >= 0
    guard = text[guard_start:current_start]
    assert "#endif" not in guard and "#else" not in guard
    for condition in ("_GLIBCXX_RELEASE == 13", "!defined(_GLIBCXX_DEBUG)",
                      "!defined(_GLIBCXX_ASSERTIONS)", "!defined(_GLIBCXX_PARALLEL)",
                      "!defined(__NO_MYSQL__)", "__cplusplus == 202002L"):
        assert contains(guard, condition), condition
    helpers = "\n".join(body(signature) for signature in (
        "bool nq_world_add(", "bool nq_world_rows(",
        "template <class Table> bool nq_world_table("))
    project = body("[[maybe_unused]] native_quest_cost_projection_result quest_cost_prepare_project(")
    adapter = "namespace {\n" + guard + current + "\n#endif\n" + project + "\n}\n"
    completion = "\n".join(body(signature) for signature in (
        "bool quest_native_completion_owner::reserve_cost_projection(",
        "native_quest_cost_projection_result quest_native_completion_owner::project_cost_projection("))
    substitutions = {
        "@QUEST_DESCRIPTOR@": descriptor,
        "@QUEST_CENSUS_HELPERS@": guard + helpers + "\n#endif\n",
        "@QUEST_ADAPTER@": adapter,
        "@QUEST_COMPLETION_METHODS@": completion,
        "@QUEST_SELECTED_DESCRIPTOR@": selected_descriptor,
        "@QUEST_SELECTED_METHOD@": body("player_snapshot_codec_result quest_native_completion_owner::encode_selected_items("),
    }
    fixture = template.read_text(encoding="utf-8")
    for token, value in substitutions.items():
        assert fixture.count(token) == 1, token
        fixture = fixture.replace(token, value)
    assert "@QUEST_" not in fixture
    output = work / "admission_cases.cpp"
    output.write_text(fixture, encoding="utf-8", newline="\n")
    print("Quest admission extracted fixture: " + hashlib.sha256(output.read_bytes()).hexdigest())
    return output


def main():
    import resource
    preliminary_parser = argparse.ArgumentParser(add_help=False)
    preliminary_parser.add_argument("--source-root", type=Path, default=ROOT)
    preliminary, _ = preliminary_parser.parse_known_args()
    source = preliminary.source_root.resolve()
    sys.path.insert(0, str(source / "tests/async"))
    from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
    from test_original_logit_source_profile import flags_from_make, check_macros
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--proposal-root", type=Path, default=ROOT)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--library-path", type=Path, action="append", default=[])
    parser.add_argument("--compile-timeout", type=positive_seconds, default=600)
    parser.add_argument("--run-timeout", type=positive_seconds, default=60)
    parser.add_argument("--selector-timeout", type=positive_seconds, default=1500)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, terminate_on_signal)
    paths = [path.resolve() for path in args.library_path]
    if any(not path.is_dir() for path in paths):
        parser.error("Each --library-path must name an existing reviewed directory")
    compiler = shutil.which(args.compiler)
    selector_compiler = shutil.which("g++")
    if not compiler or not selector_compiler or Path(compiler).resolve() != Path(selector_compiler).resolve():
        parser.error("The real selector's g++ must resolve to the selected compiler")
    linker_flags = [flag for path in paths for flag in ("-L", str(path))]
    stack = 64 * 1024 * 1024
    _, hard = resource.getrlimit(resource.RLIMIT_STACK)
    if hard != resource.RLIM_INFINITY and hard < stack:
        parser.error("The fixture requires a hard stack limit of at least 64 MiB")

    def fixture_stack():
        resource.setrlimit(resource.RLIMIT_STACK, (stack, hard))

    env = dict(os.environ)
    env.pop("DURIS_PERSISTENCE_MODE", None)
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    template = args.proposal_root.resolve() / "tests/async/quest_cost_admission_cases.cpp.in"
    assert template.is_file(), template
    selector = source / "tests/async/quest_accounting_prep/test_native_selectors.py"
    assert selector.is_file(), selector
    bin_root = source / "bin/tests"
    bin_root.mkdir(parents=True, exist_ok=True)
    probe_timeout = min(60, args.compile_timeout)
    fixture_compile_timeout = min(300, args.compile_timeout)
    with tempfile.TemporaryDirectory(prefix="quest-cost-admission-", dir=bin_root) as temporary:
        work = Path(temporary)
        fixture = generate(source, work, template)
        for backend, assertions in (("mariadb", False), ("flatfile", False), ("mariadb", True)):
            label = backend + ("-assertions" if assertions else "")
            directory = work / label
            directory.mkdir()
            ordinary = directory / "ordinary"
            objects = directory / "sectioned"
            profile_extra = ["-D_GLIBCXX_ASSERTIONS"] if assertions else []

            def make_command(objdir, extra, targets):
                return ["make", "-C", str(source / "src"), "-j2", "CC=" + args.compiler,
                        "BUILD_PROFILE=production", "PERSISTENCE_BACKEND=" + backend,
                        "OBJDIR=" + str(objdir), "DMS_BINARY=" + str(directory / "unused-server"),
                        "EXTRA_CFLAGS=" + " ".join(extra), *map(str, targets)]

            ordinary_flags = flags_from_make(
                make_command(ordinary, profile_extra, [ordinary / "world/quest.o"]), args.compiler,
                source=source, execute=execute, timeout=probe_timeout)
            flags = flags_from_make(make_command(objects, [*profile_extra, "-ffunction-sections", "-fdata-sections"],
                                                 [objects / "economy/native_quest_cost.o"]), args.compiler,
                                    source=source, execute=execute, timeout=probe_timeout)
            assert "-ffunction-sections" not in ordinary_flags and "-fdata-sections" not in ordinary_flags
            for selected_flags in (ordinary_flags, flags):
                assert "-Og" in selected_flags and "-Werror" in selected_flags and "-D__NO_TESTS__" in selected_flags
                assert "-DTEST_MUD" not in selected_flags and "-DDURIS_PERSISTENT_TRANSPORT" in selected_flags
                assert ("-D__NO_MYSQL__" in selected_flags) == (backend == "flatfile")
            for provider in ORDINARY:
                macros = execute([args.compiler, *ordinary_flags, "-dM", "-E", "-x", "c++",
                                  str(source / "src" / (provider + ".c"))],
                                 cwd=source / "src", timeout=probe_timeout, env=env, capture_output=True)
                check_macros(macros, "assertions" if assertions else "supported")
                if provider == "item/item_movement_transaction":
                    assert "#define _GLIBCXX_NATIVE_THREAD_ID pthread_self()" in macros.splitlines()
            execute(make_command(ordinary, profile_extra, [ordinary / (name + ".o") for name in ORDINARY]),
                    cwd=source, timeout=args.compile_timeout, env=env)
            assert all((ordinary / (name + ".o")).is_file() for name in ORDINARY)
            print(f"Quest admission {label}: PASS (2 ordinary whole objects)")
            sectioned_extra = [*profile_extra, "-ffunction-sections", "-fdata-sections"]
            targets = [objects / (name + ".o") for name in PROVIDERS]
            execute(make_command(objects, sectioned_extra, targets), cwd=source,
                    timeout=args.compile_timeout, env=env)
            assert all(target.is_file() for target in targets)
            compiled = directory / "admission.o"
            fixture_macros = execute([args.compiler, *flags, "-dM", "-E", "-x", "c++", str(fixture)],
                                     cwd=source / "src", timeout=probe_timeout, env=env, capture_output=True)
            check_macros(fixture_macros, "assertions" if assertions else "supported")
            assert "#define _GLIBCXX_NATIVE_THREAD_ID pthread_self()" in fixture_macros.splitlines()
            execute([args.compiler, *flags, "-c", str(fixture), "-o", str(compiled)],
                    cwd=source / "src", timeout=fixture_compile_timeout, env=env)
            binary = directory / "admission"
            execute([args.compiler, "-no-pie", str(compiled), *map(str, targets), "-Wl,--gc-sections",
                     "-pthread", *linker_flags, "-lcrypto", "-lz", "-l:libpcre2-8.so.0", "-lmariadb", "-o", str(binary)],
                    cwd=source, timeout=fixture_compile_timeout, env=env)
            output = execute([str(binary)], cwd=source, timeout=args.run_timeout, env=env,
                             preexec_fn=fixture_stack, capture_output=True)
            reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                       if line.startswith("QUEST_COST_ADMISSION ")]
            assert len(reports) == 1, reports
            report = reports[0]
            selected_reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                                if line.startswith("QUEST_SELECTED_LIST ")]
            assert len(selected_reports) == 1, selected_reports
            selected_report = selected_reports[0]
            if backend == "mariadb" and not assertions:
                assert report["success"] and report["refusal_points"] > 0 and report["no_retained_writes"], report
                assert report["cap_boundary"] and report["registered_states"] == 8, report
                assert selected_report["wire_cases"] == 2 and selected_report["invalid_cases"] == 4
                assert selected_report["parent_cap_boundaries"] and selected_report["capacity_inputs"]
                assert selected_report["registered_states"] == 8 and selected_report["child_refusal_points"] > 0
                assert selected_report["no_retained_writes"] and selected_report["strong_output"]
                selected = [objects / (name + ".o") for name in ("economy/native_quest_cost", "persistence/critical_command")]
                command = [sys.executable, str(selector), *[word for path in selected for word in ("--cost-object", str(path))],
                           *[word for path in paths for word in ("--library-path", str(path))]]
                execute(command, cwd=source, timeout=args.selector_timeout, env=env, preexec_fn=fixture_stack)
                print("Quest admission current native selectors: PASS")
            else:
                assert report == {"unsupported_no_writes": True}, report
                assert selected_report == {"unsupported_no_writes": True}, selected_report
            print(output, end="")
            print(f"Quest admission {label}: PASS")
    print("Quest cost admission: PASS (3 profiles; final projection component and current selectors)")


if __name__ == "__main__":
    main()
