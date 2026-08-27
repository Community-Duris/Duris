#!/usr/bin/env python3
"""Generate deterministic direct dependency inventory and SPDX 2.3 output."""

import argparse
import datetime as dt
import hashlib
import json
import re
import subprocess
from pathlib import Path
from urllib.parse import quote


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "packaging/duris-build-deps.equivs"


def command(*args: str) -> str:
    return subprocess.run(args, check=True, text=True, capture_output=True).stdout.strip()


def dependency_expressions() -> list[str]:
    body = MANIFEST.read_text(encoding="utf-8")
    match = re.search(r"^Depends:\s*(.*?)(?=^[A-Z][A-Za-z-]*:)", body, re.MULTILINE | re.DOTALL)
    if not match:
        raise SystemExit("dependency manifest has no Depends field")
    return sorted(" ".join(value.split()) for value in match.group(1).split(",") if value.strip())


def package_name(alternative: str) -> str:
    match = re.match(r"^([a-z0-9][a-z0-9+.-]*)", alternative.strip())
    if not match:
        raise SystemExit(f"unsupported dependency expression: {alternative}")
    return match.group(1)


def installed_package(name: str) -> tuple[str, str] | None:
    result = subprocess.run(
        ["dpkg-query", "-W", "-f=${db:Status-Status}\t${Version}\t${Architecture}", name],
        text=True,
        capture_output=True,
    )
    if result.returncode != 0:
        return None
    status, separator, remainder = result.stdout.strip().partition("\t")
    if not separator or status != "installed":
        return None
    version, separator, architecture = remainder.partition("\t")
    return (version, architecture) if separator and version and architecture else None


def installed_provider(name: str) -> tuple[str, str, str] | None:
    result = subprocess.run(
        ["dpkg-query", "-W", "-f=${Package}\t${Version}\t${Architecture}\t${Provides}\\n"],
        text=True,
        capture_output=True,
        check=True,
    )
    providers = []
    for line in result.stdout.splitlines():
        package, separator, remainder = line.partition("\t")
        if not separator:
            continue
        version, separator, remainder = remainder.partition("\t")
        if not separator:
            continue
        architecture, separator, provided = remainder.partition("\t")
        if not separator:
            continue
        names = [value.strip().split()[0] for value in provided.split(",") if value.strip()]
        if name in names:
            providers.append((package, version, architecture))
    return sorted(providers)[0] if providers else None


def resolved_inventory() -> list[dict[str, object]]:
    inventory = []
    for expression in dependency_expressions():
        alternatives = [package_name(value) for value in expression.split("|")]
        selected = None
        version = None
        architecture = None
        for name in alternatives:
            candidate = installed_package(name)
            if candidate:
                selected = name
                version, architecture = candidate
                break
            provider = installed_provider(name)
            if provider:
                selected, version, architecture = provider
                break
        inventory.append(
            {
                "alternatives": alternatives,
                "architecture": architecture,
                "declared": expression,
                "selected": selected,
                "status": "resolved" if selected else "unresolved",
                "version": version,
            }
        )
    return inventory


def project_version() -> str:
    match = re.search(r"\*\*Version: ([^*]+)\*\*", (ROOT / "README.md").read_text())
    if not match:
        raise SystemExit("README version marker is missing")
    return match.group(1).strip()


def source_timestamp() -> str:
    epoch = int(command("git", "log", "-1", "--format=%ct"))
    return dt.datetime.fromtimestamp(epoch, tz=dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def spdx_id(name: str) -> str:
    return "SPDXRef-Package-" + re.sub(r"[^A-Za-z0-9.-]", "-", name)


def os_release() -> dict[str, str]:
    values = {}
    for line in Path("/etc/os-release").read_text().splitlines():
        key, separator, value = line.partition("=")
        if separator:
            values[key] = value.strip().strip('"')
    if not values.get("ID") or not values.get("VERSION_ID"):
        raise SystemExit("/etc/os-release lacks ID or VERSION_ID")
    return values


def prepare_scanner_root(rootfs_path: Path) -> None:
    allowed = {
        Path("etc"),
        Path("etc/debian_version"),
        Path("etc/lsb-release"),
        Path("etc/os-release"),
        Path("var"),
        Path("var/lib"),
        Path("var/lib/dpkg"),
        Path("var/lib/dpkg/status"),
    }
    if not rootfs_path.name.startswith("scanner-rootfs"):
        raise SystemExit("scanner root output must use a scanner-rootfs* directory")
    if rootfs_path.is_symlink():
        raise SystemExit("scanner root output must not be a symbolic link")
    if rootfs_path.exists() and not rootfs_path.is_dir():
        raise SystemExit("scanner root output must be a directory")
    if rootfs_path.exists():
        for entry in rootfs_path.rglob("*"):
            relative = entry.relative_to(rootfs_path)
            if entry.is_symlink() or relative not in allowed:
                raise SystemExit(f"scanner root contains unmanaged path: {relative}")


def generate(inventory_path: Path, spdx_path: Path, rootfs_path: Path) -> None:
    dependencies = resolved_inventory()
    version = project_version()
    created = source_timestamp()
    inventory = {
        "generated_at": created,
        "manifest": "packaging/duris-build-deps.equivs",
        "project": "DurisMUD",
        "project_version": version,
        "scope": "direct Debian/Ubuntu build, test, and local-runtime dependencies",
        "dependencies": dependencies,
        "coverage": {
            "included": "declared direct package expressions and installed alternative versions",
            "not_included": "transitive packages, deployment-only services, firmware, and vulnerability status",
        },
    }
    canonical = json.dumps(inventory, sort_keys=True, separators=(",", ":")).encode()
    namespace_hash = hashlib.sha256(canonical).hexdigest()

    packages = [
        {
            "SPDXID": "SPDXRef-Package-DurisMUD",
            "name": "DurisMUD",
            "versionInfo": version,
            "downloadLocation": "NOASSERTION",
            "filesAnalyzed": False,
            "licenseConcluded": "NOASSERTION",
            "licenseDeclared": "NOASSERTION",
            "copyrightText": "NOASSERTION",
        }
    ]
    relationships = [
        {
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relationshipType": "DESCRIBES",
            "relatedSpdxElement": "SPDXRef-Package-DurisMUD",
        }
    ]
    for dependency in dependencies:
        if dependency["status"] != "resolved":
            continue
        name = str(dependency["selected"])
        identifier = spdx_id(name)
        purl_name = quote(name, safe=".+~-")
        purl_version = quote(str(dependency["version"]), safe=".+~-")
        purl_architecture = quote(str(dependency["architecture"]), safe=".+~-")
        packages.append(
            {
                "SPDXID": identifier,
                "name": name,
                "versionInfo": dependency["version"],
                "downloadLocation": "NOASSERTION",
                "filesAnalyzed": False,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "NOASSERTION",
                "copyrightText": "NOASSERTION",
                "externalRefs": [
                    {
                        "referenceCategory": "PACKAGE-MANAGER",
                        "referenceType": "purl",
                        "referenceLocator": (
                            f"pkg:deb/ubuntu/{purl_name}@{purl_version}?arch={purl_architecture}"
                        ),
                    }
                ],
            }
        )
        relationships.append(
            {
                "spdxElementId": "SPDXRef-Package-DurisMUD",
                "relationshipType": "DEPENDS_ON",
                "relatedSpdxElement": identifier,
            }
        )

    spdx = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"DurisMUD-{version}-direct-dependencies",
        "documentNamespace": f"https://github.com/LuminariMUD/DurisMUD/sbom/{namespace_hash}",
        "creationInfo": {"created": created, "creators": ["Tool: generate_security_sbom.py"]},
        "documentDescribes": ["SPDXRef-Package-DurisMUD"],
        "packages": packages,
        "relationships": relationships,
        "annotations": [
            {
                "annotationDate": created,
                "annotationType": "OTHER",
                "annotator": "Tool: generate_security_sbom.py",
                "comment": "Direct declared dependencies only; absence of a package or finding is not vulnerability assurance.",
            }
        ],
    }

    inventory_path.parent.mkdir(parents=True, exist_ok=True)
    spdx_path.parent.mkdir(parents=True, exist_ok=True)
    inventory_path.write_text(json.dumps(inventory, indent=2, sort_keys=True) + "\n")
    spdx_path.write_text(json.dumps(spdx, indent=2, sort_keys=True) + "\n")

    prepare_scanner_root(rootfs_path)
    identity = os_release()
    (rootfs_path / "etc").mkdir(parents=True, exist_ok=True)
    (rootfs_path / "var/lib/dpkg").mkdir(parents=True, exist_ok=True)
    (rootfs_path / "etc/os-release").write_text(
        f'ID={identity["ID"]}\nVERSION_ID="{identity["VERSION_ID"]}"\n'
    )
    (rootfs_path / "etc/lsb-release").write_text(
        f'DISTRIB_ID={identity["ID"].title()}\nDISTRIB_RELEASE={identity["VERSION_ID"]}\n'
    )
    (rootfs_path / "etc/debian_version").write_text(f'{identity["VERSION_ID"]}\n')
    paragraphs = []
    for dependency in dependencies:
        if dependency["status"] == "resolved":
            paragraphs.append(
                f'Package: {dependency["selected"]}\n'
                "Status: install ok installed\n"
                f'Architecture: {dependency["architecture"]}\n'
                f'Version: {dependency["version"]}\n'
            )
    (rootfs_path / "var/lib/dpkg/status").write_text("\n".join(paragraphs))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", type=Path, default=ROOT / "bin/security/dependency-inventory.json")
    parser.add_argument("--spdx", type=Path, default=ROOT / "bin/security/duris.spdx.json")
    parser.add_argument("--rootfs", type=Path, default=ROOT / "bin/security/scanner-rootfs")
    args = parser.parse_args()
    generate(args.inventory, args.spdx, args.rootfs)
    print(f"wrote {args.inventory}")
    print(f"wrote {args.spdx}")
    print(f"wrote {args.rootfs}")


if __name__ == "__main__":
    main()
