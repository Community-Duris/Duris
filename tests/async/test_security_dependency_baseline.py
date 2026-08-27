#!/usr/bin/env python3
"""Contracts for security policy, dependency inventory, SPDX, and CI gates."""

import json
import re
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
security = (ROOT / "SECURITY.md").read_text()
baseline = (ROOT / "docs/operations/SECURITY_BASELINE.md").read_text()
dependabot = (ROOT / ".github/dependabot.yml").read_text()
build_workflow = (ROOT / ".github/workflows/build.yml").read_text()
security_workflow = (ROOT / ".github/workflows/security.yml").read_text()
makefile = (ROOT / "Makefile").read_text()
security_checker = (ROOT / "scripts/security_source_check.py").read_bytes()


assert "security/advisories/new" in security
assert "three business days" in security and "ten business days" in security
assert re.search(r'Security\s+contact requested', security)
assert "Do not include real player data" in security
assert "Use this section to tell people" not in security
assert "not assurance" in security.lower()
print("[PASS] vulnerability policy has a private route, expectations, and safe scope")

assert 'package-ecosystem: ""' not in dependabot
assert "package-ecosystem: github-actions" in dependabot
assert re.search(r'interval:\s*["\']?weekly["\']?', dependabot)
assert "equivs" in baseline and "no ecosystem" in baseline
print("[PASS] Dependabot covers the applicable GitHub Actions ecosystem only")

uses = re.findall(r"\buses:\s*([^\s]+)", build_workflow + "\n" + security_workflow)
assert uses
for action in uses:
    assert re.search(r"@[0-9a-f]{40}$", action), f"mutable action reference: {action}"
for expected in (
    "github/codeql-action/init@",
    "github/codeql-action/analyze@",
    "aquasecurity/trivy-action@",
    "actions/upload-artifact@",
):
    assert expected in security_workflow
assert "version: v0.70.0" in security_workflow
assert "scan-type: rootfs" in security_workflow
assert "scan-ref: bin/security/scanner-rootfs" in security_workflow
assert "severity: HIGH,CRITICAL" in security_workflow
assert "ignore-unfixed: true" in security_workflow
assert "continue-on-error: true" in security_workflow
assert "Trivy did not scan a supported dependency target." in security_workflow
assert 'TRIVY_OUTCOME: ${{ steps.trivy.outcome }}' in security_workflow
assert "if: always()" in security_workflow
print("[PASS] immutable CodeQL/Trivy CI preserves reports and enforces stated policy")

assert b"BEGIN " + b"PRIVATE KEY" not in security_checker
assert b"BEGIN RSA " + b"PRIVATE KEY" not in security_checker
print("[PASS] private-key detection does not match the tracked checker itself")

assert re.search(r"^security-sbom:\s*$", makefile, re.MULTILINE)
assert re.search(r"^security-check:\s*security-sbom\s*$", makefile, re.MULTILINE)
assert "scripts/generate_security_sbom.py" in makefile
assert "scripts/security_source_check.py" in makefile

with tempfile.TemporaryDirectory(prefix="duris-security-baseline-") as temp_dir:
    first_inventory = Path(temp_dir) / "inventory-1.json"
    first_spdx = Path(temp_dir) / "sbom-1.json"
    first_rootfs = Path(temp_dir) / "scanner-rootfs-1"
    second_inventory = Path(temp_dir) / "inventory-2.json"
    second_spdx = Path(temp_dir) / "sbom-2.json"
    second_rootfs = Path(temp_dir) / "scanner-rootfs-2"
    for inventory, spdx, rootfs in (
        (first_inventory, first_spdx, first_rootfs),
        (second_inventory, second_spdx, second_rootfs),
    ):
        subprocess.run(
            [
                "python3",
                "scripts/generate_security_sbom.py",
                "--inventory",
                str(inventory),
                "--spdx",
                str(spdx),
                "--rootfs",
                str(rootfs),
            ],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
    assert first_inventory.read_bytes() == second_inventory.read_bytes()
    assert first_spdx.read_bytes() == second_spdx.read_bytes()
    assert (first_rootfs / "etc/os-release").read_bytes() == (second_rootfs / "etc/os-release").read_bytes()
    assert (first_rootfs / "etc/lsb-release").read_bytes() == (second_rootfs / "etc/lsb-release").read_bytes()
    assert (first_rootfs / "var/lib/dpkg/status").read_bytes() == (second_rootfs / "var/lib/dpkg/status").read_bytes()
    inventory = json.loads(first_inventory.read_text())
    spdx = json.loads(first_spdx.read_text())

    guarded_rootfs = Path(temp_dir) / "scanner-rootfs-guarded"
    guarded_rootfs.mkdir()
    unmanaged = guarded_rootfs / "keep-me"
    unmanaged.write_text("owned by caller\n")
    rejected = subprocess.run(
        [
            "python3",
            "scripts/generate_security_sbom.py",
            "--inventory",
            str(Path(temp_dir) / "guarded-inventory.json"),
            "--spdx",
            str(Path(temp_dir) / "guarded-sbom.json"),
            "--rootfs",
            str(guarded_rootfs),
        ],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    assert rejected.returncode != 0
    assert unmanaged.read_text() == "owned by caller\n"

assert inventory["manifest"] == "packaging/duris-build-deps.equivs"
assert inventory["dependencies"] == sorted(
    inventory["dependencies"], key=lambda dependency: dependency["declared"]
)
assert all(dependency["status"] in {"resolved", "unresolved"} for dependency in inventory["dependencies"])
assert all(dependency["alternatives"] for dependency in inventory["dependencies"])
assert all(
    dependency["architecture"]
    for dependency in inventory["dependencies"]
    if dependency["status"] == "resolved"
)
assert "vulnerability status" in inventory["coverage"]["not_included"]
assert spdx["spdxVersion"] == "SPDX-2.3"
assert spdx["dataLicense"] == "CC0-1.0"
assert spdx["documentNamespace"].startswith("https://github.com/LuminariMUD/DurisMUD/sbom/")
assert spdx["packages"][0]["name"] == "DurisMUD"
package_purls = [
    reference["referenceLocator"]
    for package in spdx["packages"]
    for reference in package.get("externalRefs", [])
]
assert package_purls
assert all("@1:" not in purl for purl in package_purls)
assert any("%3A" in purl for purl in package_purls)
assert any(relationship["relationshipType"] == "DEPENDS_ON" for relationship in spdx["relationships"])
assert "not vulnerability assurance" in spdx["annotations"][0]["comment"]
print("[PASS] direct inventory and SPDX 2.3 output are deterministic and scope-explicit")

for generated in (
    "bin/security/dependency-inventory.json",
    "bin/security/duris.spdx.json",
    "bin/security/scanner-rootfs/var/lib/dpkg/status",
):
    ignored = subprocess.run(
        ["git", "check-ignore", "-q", generated], cwd=ROOT, check=False
    )
    assert ignored.returncode == 0, f"generated artifact is not ignored: {generated}"

assert "UNKNOWN" in baseline
assert "Transitive and deployment dependency vulnerability status" in baseline
assert "Repository maintainers own triage" in baseline
assert "owner" in baseline and "expiry date" in baseline
print("[PASS] generated outputs are ignored and baseline avoids unsupported assurance")

print("security and dependency baseline contracts passed")
