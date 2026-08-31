#!/usr/bin/env python3
"""Source contracts for the complete local Docker deployment path."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
dockerfile = (ROOT / "Dockerfile").read_text()
compose = (ROOT / "compose.yaml").read_text()
entrypoint = (ROOT / "deploy/docker/entrypoint.sh").read_text()
initializer = (ROOT / "scripts/init_docker_env.sh").read_text()
socket_client = (ROOT / "scripts/mysql_socket_bin/mysql").read_text()
dockerignore = (ROOT / ".dockerignore").read_text()
gitignore = (ROOT / ".gitignore").read_text()
guide = (ROOT / "docs/operations/DOCKER.md").read_text()
runtime_verifier = (ROOT / "migrations/verify_runtime_compatibility.sh").read_text()


def mapping_entry(source: str, key: str, indent: int) -> str:
    """Return one indentation-delimited YAML mapping entry."""
    lines = source.splitlines()
    header = f"{' ' * indent}{key}:"
    start = lines.index(header)
    end = len(lines)
    for index in range(start + 1, len(lines)):
        stripped = lines[index].lstrip()
        if stripped and not stripped.startswith("#"):
            line_indent = len(lines[index]) - len(stripped)
            if line_indent <= indent:
                end = index
                break
    return "\n".join(lines[start:end])


mariadb_service = mapping_entry(compose, "mariadb", 2)
game_service = mapping_entry(compose, "game", 2)

# The runtime image is built from source but runs without root privileges.
assert dockerfile.count("FROM ubuntu:24.04") == 2
assert "make -j\"${BUILD_JOBS}\" build-server build-area-tools" in dockerfile
assert "USER duris" in dockerfile
assert 'ENTRYPOINT ["./deploy/docker/entrypoint.sh"]' in dockerfile
assert "./scripts/healthcheck.sh" in dockerfile
assert "--retries=30" in dockerfile
assert "rm -rf bin/objects" in dockerfile
assert "\n        /opt/duris/Players \\" in dockerfile
assert "/opt/duris/Players/Tradeskills" in dockerfile
assert '"/opt/duris/Players/Tradeskills/$letter"' in dockerfile

# Compose owns a fresh-schema database and app, without publishing MariaDB.
assert "mariadb:" in compose and "game:" in compose
assert "migrations/bootstrap_multithread_safe.sql" in compose
assert "condition: service_healthy" in compose
assert "mariadb-data:/var/lib/mysql" in compose
assert "mariadb-socket:/run/mysqld:ro" in compose
assert "duris-players:/opt/duris/Players" in compose
assert "DB_SOCKET: /run/mysqld/mysqld.sock" in compose
assert "DB_HOST: localhost" in compose
assert "DB_ALLOWED_TARGETS: localhost/duris_dev" in compose
assert 'DURIS_DOCKER_BIND_ADDRESS:-127.0.0.1' in compose
assert "ports:" not in mariadb_service
assert 'restart: "on-failure:5"' in game_service
assert "no-new-privileges:true" in mariadb_service
assert "no-new-privileges:true" in game_service

# First boot seeds and adopts only an empty-baseline fresh schema, then reuses
# the existing guarded launcher for migrations, backups, world generation, and boot.
assert "SELECT COUNT(*) FROM mud_schema_baselines" in entrypoint
assert "import_help_to_prod.sh --local" in entrypoint
assert "migration_runner.py adopt --kind fresh_bootstrap" in entrypoint
assert "exec ./scripts/cycle_mud.sh --dev" in entrypoint
assert "Unexpected migration baseline count" in entrypoint
assert 'if ! baseline_count="$(' in entrypoint
assert "Unable to query mud_schema_baselines" in entrypoint
assert "import_help_to_prod.sh --local < <(yes yes)" in entrypoint
assert "--protocol=socket" in socket_client
assert "-h|-P|--host|--port|--protocol|--socket" in socket_client

socket_verifier_branch = runtime_verifier.split('if [[ -n "${DB_SOCKET:-}" ]]', 1)[1].split(
    "else", 1
)[0]
assert "MYSQL_SSL" not in socket_verifier_branch
assert 'MYSQL_CONNECTION=("${MYSQL_SSL[@]}" -h "$DB_HOST"' in runtime_verifier

# Secrets stay outside both Git and the Docker build context, and initialization
# is one-shot rather than silently replacing a live stack's credentials.
assert ".env.docker*" in gitignore
assert ".env.*" in dockerignore
assert "!Players/.gitkeep" not in dockerignore
assert "Refusing to overwrite existing Docker configuration" in initializer
assert initializer.count("secrets.token_hex(32)") == 2
assert "chmod 0600" in initializer

for command in (
    "./scripts/init_docker_env.sh",
    "docker compose --env-file .env.docker up --build --detach --wait",
    "docker compose --env-file .env.docker down --volumes",
):
    assert command in guide

assert "persistent named volume" in guide
assert "duris-players" in guide

print("Docker deployment contracts passed")
