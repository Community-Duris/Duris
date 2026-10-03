#!/usr/bin/env python3
"""Run the reviewed SQL/recovery workload using only owned disposable fixtures."""
from __future__ import annotations

import argparse
from contextlib import nullcontext
from dataclasses import replace
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tests/async"))
from disposable_sql_fixture import DisposableSQL, clean_environment, image_identity
from regression_inventory import inventory
from run_regression_tests import run_test, skip_count, TestSpec, terminate_test

MANIFEST = ROOT / "tests/integration_manifest.json"


def atomic_json(path, value):
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    temporary.replace(path)


def write_report(path, report):
    atomic_json(path, report)
    rows = list(report["attempts"])
    rows += [dict(engine=identity.split("/", 1)[0], row=identity.split("/", 1)[1],
                  status="incomplete", failures=["required row has not completed"])
             for identity in report["pending"]]
    xml = ET.Element("testsuite", name="disposable-integration-matrix", tests=str(len(rows)),
                     failures=str(sum(row["status"] != "passed" for row in rows)), skipped="0")
    for row in rows:
        case = ET.SubElement(xml, "testcase", classname=row["engine"], name=row["row"],
                             time=str(row.get("elapsed", 0)))
        if row["status"] != "passed":
            ET.SubElement(case, "failure", type=row["status"]).text = "\n".join(row["failures"])
    target = path.with_suffix(".xml")
    temporary = target.with_suffix(".xml.tmp")
    ET.ElementTree(xml).write(temporary, encoding="utf-8", xml_declaration=True)
    temporary.replace(target)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def workload(document, specs):
    """Validate coverage before selection; removing a required owner cannot hide it."""
    if document["version"] != 1:
        raise ValueError("unsupported integration workload")
    rows = document["rows"]
    ids = [row["id"] for row in rows]
    if len(ids) != len(set(ids)):
        raise ValueError("duplicate integration row identity")
    covered = {name for row in rows for name in row["covers"]}
    required = {spec.path.name for spec in specs if spec.manual or
                spec.profile in {"database", "recovery"} or "database" in spec.also_profiles}
    if required - covered:
        raise ValueError("integration workload omits required owners: " + ", ".join(sorted(required - covered)))
    for row in rows:
        path = (ROOT / row["path"]).resolve()
        if (not path.is_relative_to(ROOT / "tests") or not path.is_file()
                or row["provider"] not in {"sql", "self-sql", "offline", "non-root", "privileged-recovery"}
                or not row["required_cases"] or len(set(row["required_cases"])) != len(row["required_cases"])
                or row["mode"] not in {"script", "unittest"} or row["timeout_seconds"] <= 0
                or any(engine not in {*document["engines"], "once"} for engine in row["engines"])):
            raise ValueError("invalid integration row: " + row["id"])
    return rows


def freeze_build(directory, environment):
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    if subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True).strip():
        raise RuntimeError("commit the complete source before freezing the integration batch")
    inputs = subprocess.check_output(["git", "ls-files", "src", "migrations", "Makefile"],
                                     cwd=ROOT, text=True).splitlines()
    source_manifest = directory / "build-source-manifest.json"
    atomic_json(source_manifest, {name: digest(ROOT / name) for name in inputs})
    all_inputs = subprocess.check_output(["git", "ls-files"], cwd=ROOT, text=True).splitlines()
    atomic_json(directory / "workload-source-manifest.json", {name: digest(ROOT / name)
                for name in all_inputs if (ROOT / name).is_file()})
    binaries = {"sql_binary": directory / "dms-sql", "flat_binary": directory / "dms-flatfile"}
    checks = []
    commands = [
        ["make", "-C", "src", "-j2", "PERSISTENCE_BACKEND=mariadb", "DMS_BINARY=" + str(binaries["sql_binary"])],
        ["make", "-C", "src", "-j2", "PERSISTENCE_BACKEND=flatfile", "DMS_BINARY=" + str(binaries["flat_binary"])],
        # Recovery tools and a few maintained wrappers consume the standard SQL
        # path. Link it from the same objects so a clean checkout is sufficient.
        ["make", "-C", "src", "-j2", "PERSISTENCE_BACKEND=mariadb"],
        ["make", "build-restore-tools", "world", "-j2"],
        ["python3", "tests/async/test_pa_accounting_batch_artifact.py"],
        ["python3", "tests/async/test_pa_copyover_artifact_contract.py"],
    ]
    for index, command in enumerate(commands):
        print("matrix build/check: " + " ".join(command), flush=True)
        with (directory / f"build-{index}.log").open("w") as log:
            process = subprocess.Popen(command, cwd=ROOT, env=environment, stdout=log,
                                       stderr=subprocess.STDOUT, start_new_session=True)
            try:
                exit_code = process.wait(timeout=1800)
            except BaseException:
                terminate_test(process)
                checks.append({"command": command, "exit": process.returncode, "incomplete": True})
                atomic_json(directory / "build-checks.json", checks)
                raise
        checks.append({"command": command, "exit": exit_code})
        atomic_json(directory / "build-checks.json", checks)
        if exit_code:
            raise RuntimeError("matrix build/check failed; original log: " + str(directory / f"build-{index}.log"))
    # Recheck source bytes after compilation, before declaring this artifact qualified.
    recorded = json.loads((directory / "workload-source-manifest.json").read_text())
    if any(digest(ROOT / name) != value for name, value in recorded.items()):
        raise RuntimeError("source changed while compiling frozen matrix artifacts")
    if subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip() != head:
        raise RuntimeError("source commit changed while compiling frozen matrix artifacts")
    for binary in binaries.values():
        binary.chmod(0o555)
    source_manifest.chmod(0o444)
    descriptor = directory / "base-build.json"
    atomic_json(descriptor, dict(head=head, status="BUILD_AND_LOCAL_CONTRACTS_PASS", backend="mariadb",
                                profile="development/TEST_MUD", binary=str(binaries["sql_binary"]),
                                binary_sha256=digest(binaries["sql_binary"]),
                                source_manifest=str(source_manifest), checks=checks))
    descriptor.chmod(0o444)
    compiler = subprocess.check_output([environment.get("CXX", "g++"), "--version"], text=True)
    configurations = {}
    for backend in ("mariadb", "flatfile"):
        configurations[backend] = subprocess.check_output([
            "make", "-s", "--no-print-directory", "-C", "src", "PERSISTENCE_BACKEND=" + backend,
            "--eval=.PHONY: matrix-config",
            "--eval=matrix-config:;@echo CC=$(CC); echo FLAGS=$(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS)",
            "matrix-config"], cwd=ROOT, text=True, env=environment)
    atomic_json(directory / "compiler.json", {"version": compiler, "configurations": configurations})
    recovery_tools = {tool: subprocess.check_output([tool, "--version"], text=True).strip()
                      for tool in ("mariadb", "mariadbd", "curl", "systemd-analyze")}
    # The bootstrap script does not implement --version; invoking that option
    # can initialize a database. Bind its bytes instead.
    recovery_tools["mariadb-install-db_sha256"] = digest(Path(shutil.which("mariadb-install-db")))
    mysql_server = shutil.which("mysqld", path=environment.get("PATH"))
    if mysql_server:
        resolved = Path(mysql_server).resolve()
        recovery_tools["mysql_restore_server"] = {
            "path": str(resolved), "sha256": digest(resolved),
            "version": subprocess.check_output(
                [str(resolved), "--no-defaults", "--version"], text=True).strip()}
    atomic_json(directory / "recovery-tools.json", recovery_tools)
    return {**{key: str(value) for key, value in binaries.items()}, "head": head,
            "binary_sha256": digest(binaries["sql_binary"]), "descriptor": str(descriptor)}


def outcome_contract(row, result):
    observed = {case["id"].split("::", 1)[-1]: case["status"] for case in result.cases}
    missing = set(row["required_cases"]) - observed.keys()
    bad = {case: observed[case] for case in row["required_cases"] if case in observed and observed[case] != "passed"}
    _, skipped = skip_count(result.output)
    missing_markers = [marker for marker in row["required_markers"] if marker not in result.output]
    failures = []
    if result.returncode or result.status != "passed":
        failures.append(f"entry {result.status}, exit {result.returncode}")
    if missing:
        failures.append("missing required cases: " + ", ".join(sorted(missing)))
    if bad:
        failures.append("required cases did not pass: " + json.dumps(bad, sort_keys=True))
    if result.skipped_checks or skipped:
        failures.append("required integration checks skipped")
    if missing_markers:
        failures.append("missing requirement evidence: " + ", ".join(missing_markers))
    return failures


def run_row(row, tokens, environment, specs):
    evidence = Path(tokens["evidence"])
    evidence.mkdir(parents=True)
    arguments = [value.format(**tokens) for value in row["arguments"]]
    environment = dict(environment, **{key: value.format(**tokens) for key, value in row["environment"].items()
                                      if key != "schema_suffix"})
    environment["DURIS_MATRIX_ROW_EVIDENCE"] = tokens["evidence"]
    path = ROOT / row["path"]
    if path.suffix == ".sh":
        arguments = [row["path"], *arguments]
        path = ROOT / "tests/integration_shell_entry.py"
    spec = specs.get(Path(row["path"]).name, TestSpec(path=path))
    spec = replace(spec, path=path, mode=row["mode"], minimum_cases=len(row["required_cases"])
                   if row["mode"] == "unittest" else 1, manual=False)
    prefix, uid = (), None
    if row["provider"] == "non-root" and os.geteuid() == 0:
        prefix, uid = ("runuser", "--user", "nobody", "--"), 65534
    if row["provider"] == "privileged-recovery":
        subprocess.run(["unshare", "--user", "--map-root-user", "--net", "true"], check=True,
                       env=environment, timeout=20)
        if os.geteuid() != 0:
            raise RuntimeError("all backup cases require root in an owned namespace/container or sudo")
    started = time.time()
    result = run_test(path, timeout=row["timeout_seconds"], spec=spec, arguments=arguments,
                      environment=environment, prefix=prefix, observer_uid=uid)
    output = result.output
    for key, secret in environment.items():
        if secret and any(word in key.upper() for word in ("PASS", "PWD", "SECRET", "TOKEN")):
            output = output.replace(secret, "<fixture-secret>")
    (evidence / "original.log").write_text(output)
    failures = outcome_contract(row, result)
    record = dict(row=row["id"], source_path=row["path"], provider=row["provider"],
                  engine=tokens["engine"], required_cases=row["required_cases"],
                  cases=result.cases, started=started, elapsed=result.elapsed,
                  returncode=result.returncode, skipped_checks=result.skipped_checks,
                  status="failed" if failures else "passed", failures=failures,
                  evidence=str(evidence))
    atomic_json(evidence / "result.json", record)
    return record


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", choices=("mysql", "mariadb", "once"))
    parser.add_argument("--match", action="append",
                        help="select rows matching any repeated substring filter")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--evidence-root", type=Path, default=ROOT / "bin/integration-results")
    parser.add_argument("--tools-image", default=os.environ.get("DURIS_TEST_TOOLS_IMAGE", "duris-regression-tools:local"))
    args = parser.parse_args(argv)
    specs = inventory(ROOT / "tests/async", ROOT / "tests/regression_manifest.json")
    document = json.loads(MANIFEST.read_text())
    rows = workload(document, specs)
    selected = [(engine, row) for engine in (*document["engines"], "once") for row in rows
                if engine in row["engines"] and (not args.engine or args.engine == engine)
                and (not args.match or any(value in row["id"] for value in args.match))]
    if not selected:
        parser.error("no matrix rows selected")
    if args.list:
        for engine, row in selected:
            print(f"{engine:7} {row['id']:55} {row['provider']:20} required={len(row['required_cases'])}")
        return 0
    if sys.platform != "linux":
        parser.error("matrix executes Linux fixtures; use the documented owned test container")
    directory = args.evidence_root.resolve() / (time.strftime("%Y%m%d-%H%M%S") + "-" + os.urandom(4).hex())
    directory.mkdir(parents=True)
    report_selected = [f"{engine}/{row['id']}" for engine, row in selected]
    report = dict(version=1, status="running", attempts=[], pending=list(report_selected),
                  selected=report_selected,
                  excluded=[f"{engine}/{row['id']}" for engine in (*document["engines"], "once") for row in rows
                            if engine in row["engines"] and f"{engine}/{row['id']}" not in report_selected],
                  workload_sha256=digest(MANIFEST), tools_image_requested=args.tools_image)
    report_path = directory / "matrix.json"
    write_report(report_path, report)
    environment = dict(clean_environment(), CXX=os.environ.get("CXX", "g++"),
                       DURIS_TEST_TOOLS_IMAGE=args.tools_image)
    try:
        report["tools_image"] = image_identity(args.tools_image)
        environment["DURIS_TEST_TOOLS_IMAGE"] = report["tools_image"]["id"]
        report["source_head"] = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
        write_report(report_path, report)
        tokens = freeze_build(directory, environment)
        report.update(tokens)
        write_report(report_path, report)
        resources = directory / "resources"
        resources.mkdir()
        for name in ("test-slot-0.lock", "test-slot-1.lock", "docker-heavy.lock"):
            (resources / name).touch(mode=0o600)
        environment.update(DURIS_ACCOUNTING_BASE_BUILD=tokens["descriptor"],
                           DURIS_ACCOUNTING_RESOURCE_ROOT=str(resources),
                           DURIS_ACCOUNTING_DOCKER_HEAVY_LOCK=str(resources / "docker-heavy.lock"),
                           PA_RUNTIME_SQL_SLOT_LOCK=str(resources / "test-slot-0.lock"),
                           PA_RUNTIME_SQL_HEAVY_LOCK=str(resources / "docker-heavy.lock"),
                           PA_RUNTIME_SQL_MARIADB_IMAGE=document["engines"]["mariadb"],
                           PA_RUNTIME_SQL_MYSQL_IMAGE=document["engines"]["mysql"])
        spec_map = {spec.path.name: spec for spec in specs}
        for engine in (*document["engines"], "once"):
            engine_rows = [row for selected_engine, row in selected if selected_engine == engine]
            if not engine_rows:
                continue
            image = document["engines"].get(engine, "")
            if image:
                report.setdefault("engines", {})[engine] = image_identity(image)
            provider = DisposableSQL(image, directory / engine / "service") if any(
                row["provider"] == "sql" for row in engine_rows) else nullcontext(None)
            with provider as sql:
                for row in engine_rows:
                    identity = engine + "/" + row["id"]
                    print("matrix: " + identity, flush=True)
                    row_tokens = dict(tokens, engine=engine, image=image,
                                      engine_label="mysql8" if engine == "mysql" else "mariadb10_11",
                                      token=os.urandom(6).hex(), evidence=str(directory / engine / row["id"]))
                    row_environment = dict(environment, DURIS_TEST_DB_IMAGE=image, PA_COPYOVER_DB_IMAGE=image)
                    # Existing wrappers keep their explicit opt-ins; their reviewed image
                    # selectors receive the same pinned provider as the surrounding row.
                    text = (ROOT / row["path"]).read_text()
                    if image:
                        for variable in re.findall(r'\$\{([A-Z_]+(?:DB_IMAGE|MYSQL_IMAGE))[:-]', text):
                            row_environment[variable] = image
                    if row["provider"] == "sql":
                        row_environment.update(sql.environment)
                        if row.get("schema_prefix"):
                            schema = sql.schema(row["schema_prefix"], migrated=row["migrated"],
                                                suffix=row["environment"].get("schema_suffix", ""))
                        else:
                            schema = nullcontext(sql.environment)
                    else:
                        schema = nullcontext(row_environment)
                    try:
                        with schema as schema_environment:
                            row_environment.update(schema_environment)
                            if "TELEMETRY_REPOSITORY_DISPOSABLE" in row["environment"]:
                                row_environment.update(TELEMETRY_FIXTURE_DATABASE=row_environment["DB_NAME"],
                                                       TELEMETRY_FIXTURE_PASSWORD=row_environment["DB_PASSWD"])
                            record = run_row(row, row_tokens, row_environment, spec_map)
                    except Exception as error:
                        record = dict(row=row["id"], engine=engine, status="incomplete",
                                      failures=[sql.redact(str(error)) if sql else str(error)], provider=row["provider"])
                    report["attempts"].append(record)
                    report["pending"].remove(identity)
                    write_report(report_path, report)
                    print(f"matrix: {identity} {record['status']}", flush=True)
        report["status"] = "passed" if not report["pending"] and all(
            row["status"] == "passed" for row in report["attempts"]) else "failed"
    except BaseException as error:
        report["status"] = "incomplete"
        report["error"] = str(error)
    finally:
        write_report(report_path, report)
    print("matrix report: " + str(report_path), flush=True)
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
