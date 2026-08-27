# Environments

The runtime reads `ENVIRONMENT` and the explicit listener, database, and transport
settings from the owner-controlled `.env` file. The listener port does not select a
database.

| Environment | Listener | Database and TLS boundary |
|-------------|----------|---------------------------|
| Local development | Exact loopback address; development ports are allowed | Exact `DB_ALLOWED_TARGETS` entry; loopback TCP or local socket is allowed. |
| Network deployment | Operator-selected network address; production role requires the standard game port | Exact target allow-list plus verified TLS for a non-loopback database. |

Required database settings are `DB_HOST`, optional `DB_PORT`, `DB_USER`, `DB_PASSWD`,
`DB_NAME`, and `DB_ALLOWED_TARGETS`. Typed recovery also requires
`PLAYER_SAVE_JOURNAL_DIR` and `CRITICAL_COMMAND_JOURNAL_DIR`.

See [Configuration](CONFIGURATION.md) for every supported variable and its verified
default. No staging or production URL, host, credential, or deployment provider is
declared in this repository.
