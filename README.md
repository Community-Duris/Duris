# DurisMUD

DurisMUD forked from Xanadinn's repo.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Database Setup](#database-setup)
- [Compilation](#compilation)
- [Configuration](#configuration)
- [Running the MUD](#running-the-mud)
- [Connecting](#connecting)
- [Troubleshooting](#troubleshooting)
- [Development vs Production](#development-vs-production)
- [Database Migrations](#database-migrations)
- [Project Structure](#project-structure)
- [Logs](#logs)

---

## Prerequisites

### Required Software

- **C Compiler:** GCC 4.x or later (tested with GCC 14.2.0)
- **Build Tools:** GNU Make
- **MySQL/MariaDB:** 8.0 or later (tested with MySQL 8.0.44)
- **MySQL Client Library:** libmysqlclient-dev

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install libxml2 libxml2-dev
sudo apt-get install zlib1g zlib1g-dev
sudo apt-get install gnutls-dev
sudo apt-get install libcjson-dev libssl-dev
sudo apt-get install libhiredis-dev libbsd-dev
sudo apt-get install default-libmysqlclient-dev default-mysql-server
```
(MySQL has been replaced by MariaDB)

**CentOS/RHEL:**
```bash
sudo yum install gcc make mysql-server mysql-devel
```

---

## Database Setup

### 1. Install and Start MySQL

```bash
# Start MySQL service
sudo systemctl start mysql
sudo systemctl enable mysql
```
or
```bash
sudo service restart mysql
```

### 2. Create Database and User

**Development Database:**
```bash
mysql -u root -p
```

```sql
-- Create development database
CREATE DATABASE duris_dev;

-- Create user with privileges
GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'localhost' IDENTIFIED BY 'duris';
GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'127.0.0.1' IDENTIFIED BY 'duris';
FLUSH PRIVILEGES;

-- newer versions of MYSQL
CREATE USER 'duris'@'localhost' IDENTIFIED BY 'duris';
CREATE USER 'duris'@'127.0.0.1' IDENTIFIED BY 'duris';
GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'localhost' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;
FLUSH PRIVILEGES;
```

**Production Database (when ready):**
```sql
-- Create production database
CREATE DATABASE duris;

-- Grant privileges (use a strong password in production!)
GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'localhost' IDENTIFIED BY 'your_secure_password';
GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'127.0.0.1' IDENTIFIED BY 'your_secure_password';
FLUSH PRIVILEGES;

-- newer versions of MYSQL
CREATE USER 'duris'@'localhost' IDENTIFIED BY 'duris';
CREATE USER 'duris'@'127.0.0.1' IDENTIFIED BY 'duris';
GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'localhost' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON duris.* TO 'duris'@'127.0.0.1' WITH GRANT OPTION;
FLUSH PRIVILEGES;
```

### 3. Import Database Schema

```bash
# For development:
mysql -u duris -p duris_dev < src/duris.sql

# For production:
mysql -u duris -p duris < src/duris.sql
```

### 4. Apply Migrations

This branch does **not** use an in-process automigration feature at boot.
Run schema changes explicitly with a shell script or direct `mysql` import.

```bash
# Fresh database only: create the complete baseline schema.
mysql -u duris -p duris_dev < migrations/bootstrap_multithread_safe.sql

# Existing populated database: run the incremental upgrade path.
./migrations/run_migration.sh
```

`migrations/bootstrap_multithread_safe.sql` is the authoritative **fresh-database baseline** for this branch.

For an existing populated database, use `migrations/run_migration.sh` instead. The bootstrap uses `CREATE TABLE` definitions and is useful for schema comparison, but it is not an upgrade runner: `CREATE TABLE IF NOT EXISTS` does not add missing columns to an existing table.

The migration runner contains the additive, guarded upgrade steps and is designed to be re-runnable after clone validation. `migrations/run_migration.sh` is the single entrypoint; there is no root-level wrapper.

For a scoped persistence/auction repair on an archive-restored clone, use:

```bash
# Read-only exact-schema check.
./migrations/verify_persistence_contract.sh

# Apply only the persistence/auction contract; the confirmation must exactly
# match DB_NAME from the selected clone configuration.
./migrations/apply_persistence_contract.sh --confirm-db <clone_database_name>
```

Do not apply either migration path to an active database without first taking a backup, restoring a clone, and validating replay against that clone.

**Note:** The frag leaderboard tables will be automatically populated as players log in and save. No manual population is needed.

### 5. Import Help Files (Optional)

Help files, news, credits, etc, you can import them to the database:

```bash
# Edit the script to configure your database settings:
nano scripts/import_help_to_prod.sh
# Update: REMOTE_HOST, REMOTE_USER, MYSQL_USER, MYSQL_PASS, MYSQL_DB

# Dry run to see what would be imported:
./scripts/import_help_to_prod.sh --dry-run

# Import to database:
./scripts/import_help_to_prod.sh
```

This imports:
- Individual help files (motd, news, help, faq, etc.) → `mud_info` and `pages` tables
- Help index entries (~535 entries) → `pages` table
- Parsed help file entries → `pages` table

**Note:** This is only needed if you're running the web interface. The MUD itself reads help files directly from disk.

### 6. Link an SSL Certificate

Unless configured otherwise, the game expects the SSL cert and its private
key as `duris.crt` and `duris.key`.  It's probably most convenient to use
symlinks for managing them.

For testing, you can use a self-signed certificate.  You can link it via:
```
ln -s certs/localhost.crt duris.crt
ln -s certs/localhost.key duris.key
```

For a server reachable from the network, though, you should use a real
certificate.  You can obtain one eg. via Let's Encrypt.  Once you do,
you need to set up a cronjob to renew it, allow the Duris process to
read the files, and point the links appropriately, for example:
```
ln -s /var/lib/dehydrated/certs/testduris.net/fullchain.pem duris.crt
ln -s /var/lib/dehydrated/certs/testduris.net/privkey.pem duris.key
```

---

## Compilation

### Standard Build

```bash
cd src
make
cp dms_new ../dms
```

**Note:** The Makefile compiles to `src/dms_new`, which you then copy to `dms` in the root directory.

### Area File Bootstrap

The runtime expects generated `areas/world.*` files. On a fresh clone, the area-generator helpers may need to be rebuilt first:

```bash
cd areas/src
make -j1
cd ..
./m_slow
```

This rebuilds the missing `areas/make_*` helper binaries, then generates the combined `world.*` files used at boot.

### Clean Build

```bash
cd src
make clean
make
cp dms_new ../dms
```

### Build Configuration

The build is configured in `src/Makefile`:

- **MySQL Enabled:** MySQL support is enabled by default
- **Test Mode:** `TEST_MUD` flag is enabled for development builds

To disable MySQL (not recommended):
```bash
# Edit src/Makefile and uncomment:
# CFLAGS += -D__NO_MYSQL__
```

---

## Configuration

### Database Credentials

Database connection settings are **hardcoded** in `src/sql.h` (lines 6-16):

```c
#ifdef TEST_MUD
  #define DB_HOST "localhost"
  #define DB_USER "duris"
  #define DB_PASSWD "duris"
  #define DB_NAME "duris_dev"
#else
  #define DB_HOST "localhost"
  #define DB_USER "duris"
  #define DB_PASSWD "duris"
  #define DB_NAME "duris"
#endif
```

**Important Notes:**
- These credentials are compiled into the binary
- Changes require recompilation: `cd src && make clean && make`
- For production: Change `DB_PASSWD` to a secure password before compiling
- Default credentials: **user:** `duris`, **password:** `duris`

### Port Configuration

The MUD automatically selects the database based on the port:

- **Port 7777 (default):** Uses production database (`duris`)
- **Other ports:** Uses development database (`duris_dev`)

This is configured in `src/sql.c` `initialize_mysql()` function.

---

## Running the MUD

### Starting the Server

The helper scripts now self-anchor to the repository root, so you can launch them from anywhere as long as the checkout is intact.

**Preferred startup:**
```bash
./scripts/start_mud.sh
```

**Direct startup / debugging:**
```bash
./scripts/cycle_mud.sh
```

If the area helper binaries are missing, `scripts/cycle_mud.sh` will rebuild them automatically before running `areas/m_slow`.

**Development (port 4000):**
```bash
./dms 4000
```

**Production (port 7777):**
```bash
./dms 7777
# or just:
./dms
```

### Running in Background

```bash
# Using nohup:
nohup ./dms 7777 > logs/mud.out 2>&1 &

# Using screen:
screen -S duris
./dms 7777
# Press Ctrl+A, D to detach
```

### Stopping the Server

**Graceful shutdown:**
- Connect as immortal and use `shutdown` command

**Force stop:**
```bash
# Find process:
ps aux | grep duris

# Kill process:
kill <PID>
```

---

## Connecting

### Using telnet

```bash
telnet localhost 7777
```

### Using MUD Client

Recommended clients:
- **TinTin++** (Linux/Mac/Windows)
- **MUSHclient** (Windows)
- **Mudlet** (Cross-platform)

Connect to:
- **Host:** localhost (or your server IP)
- **Port:** 7777 (or configured port)

---

## Troubleshooting

### MySQL Initialization Failed

**Error message:**
```
MySQL initialization failed! Dying!
```

**Common causes and solutions:**

1. **MySQL not running:**
   ```bash
   sudo systemctl status mysql
   sudo systemctl start mysql
   ```
   or, more universal:
   ```
   sudo service mysql restart
   ```

2. **Database doesn't exist:**
   ```bash
   mysql -u root -p -e "CREATE DATABASE duris_dev;"
   ```

3. **Wrong credentials:**
   ```bash
   # Test connection:
   mysql -hlocalhost -u duris -p duris_dev
   # Password: duris
   ```

4. **User lacks privileges:**
   ```sql
   GRANT ALL PRIVILEGES ON duris_dev.* TO 'duris'@'localhost' IDENTIFIED BY 'duris';
   FLUSH PRIVILEGES;
   ```

5. **Database schema not loaded:**
   ```bash
   mysql -u duris -p duris_dev < src/duris.sql
   ```

### Check Logs

```bash
# Status log (MySQL connection, etc.):
tail -f logs/log/status

# System log (game events):
tail -f logs/log/syslog

# Command log (player commands):
tail -f logs/log/cmdlog
```

### Compilation Errors

**Missing MySQL library:**
```bash
sudo apt-get install libmysqlclient-dev
```

**Undefined references:**
```bash
# Clean and rebuild:
cd src
make clean
make
```

---

## Development vs Production

### Development Mode

**Characteristics:**
- Uses `duris_dev` database
- Runs on non-7777 ports (e.g., 4000, 4001)
- `TEST_MUD` flag enabled in compilation
- Safe for testing without affecting production

**Running:**
```bash
./dms 4000
```

### Production Mode

**Characteristics:**
- Uses `duris` database
- Runs on port 7777 (default)
- Production-grade database credentials recommended
- Affects live player data

**Running:**
```bash
./dms 7777
```

**Security recommendations:**
- Change database password from default `duris`
- Update `src/sql.h` with secure credentials
- Recompile after credential changes
- Use firewall rules to restrict MySQL access
- Regular database backups

---

## Database Migrations

### Applying Migrations

Migrations are SQL scripts in `sql/migrations/` directory.

**To apply a migration:**
```bash
# Development:
mysql -u duris -p duris_dev < sql/migrations/migration_name.sql

# Production:
mysql -u duris -p duris < sql/migrations/migration_name.sql
```

### Available Migrations

1. **add_frag_leaderboard_tables.sql**
   - Adds `account_characters` and `frag_leaderboard` tables
   - Required for web leaderboard integration
   - Safe to run multiple times (uses `IF NOT EXISTS`)
   - Tables will automatically populate as players log in and save

### Creating New Migrations

1. Create SQL file in `sql/migrations/`
2. Use descriptive filename (e.g., `add_feature_name.sql`)
3. Include header comment with purpose and date
4. Use `IF NOT EXISTS` or `IF EXISTS` for idempotency
5. Test on development database first

---

## Project Structure

```
DurisMUD/
├── src/               # C source code
│   ├── sql.c          # MySQL integration
│   ├── sql.h          # Database configuration
│   ├── files.c        # Player file I/O
│   ├── comm.c         # Network communication
│   ├── nanny.c        # Login handler
│   └── ...
├── src-migrate/       # Standalone migration/conversion tools (pfile_converter, ...)
├── lib/               # Game data files
│   ├── information/   # Help files
│   ├── etc/           # Configuration files
│   └── ...
├── areas/             # Zone/area files
├── Players/           # Player save files
│   └── {a-z}/         # Organized by first letter
├── Accounts/          # Account files
│   └── {a-z}/         # Organized by first letter
├── logs/              # Log files
│   └── log/           # System logs
│       ├── status     # MySQL and system status
│       ├── syslog     # Game events
│       └── cmdlog     # Player commands
├── migrations/        # Database migrations (authoritative runner lives here)
├── scripts/           # Operational scripts (start_mud.sh, cycle_mud.sh, gdbdms, ...)
├── help/              # Help source files (*.hlp) and the help style guide
├── certs/             # Self-signed/testing certificates
├── packaging/         # Build-dependency packaging (equivs)
├── archives/          # Seed/backup tarballs
├── attic/             # Retired areas, lib data and old_code/
├── docs/              # Documentation
├── tests/             # Test suites
└── dms                # Compiled binary
```

Scripts under `scripts/` cd to the repository root themselves, so they can be
run from anywhere (`./scripts/start_mud.sh`).


### Important Files

- **dms** - Compiled MUD executable
- **src/sql.h** - Database credentials and configuration
- **src/config.h** - MUD configuration (port, directories, etc.)
- **src/duris.sql** - Main database schema
- **logs/log/status** - MySQL connection logs
- **logs/log/syslog** - Main game log

---

## Logs

### Log Locations

All logs are in `logs/log/` directory:

| Log File | Purpose | When to Check |
|----------|---------|---------------|
| **status** | MySQL connections, system status | Database connection issues |
| **syslog** | Game events, player actions | General debugging |
| **cmdlog** | Player commands | Command debugging |
| **wizlog** | Immortal commands | Admin actions |

### Viewing Logs

```bash
# Real-time monitoring:
tail -f logs/log/status
tail -f logs/log/syslog

# Search for errors:
grep -i error logs/log/status
grep -i mysql logs/log/status

# View recent entries:
tail -100 logs/log/status
```

---

## Contributing

### Code Style

- Use consistent indentation (2 spaces)
- Comment complex logic
- Follow existing code patterns
- Test thoroughly before committing

### Committing Changes

```bash
# Create a new feature branch:
git checkout -b feature/your-feature-name

# Stage changes:
git add <files>

# Commit with descriptive message:
git commit -m "Brief description of changes"

# Push branch to remote:
git push origin feature/your-feature-name

# Create pull request using GitHub CLI:
gh pr create --title "Your feature title" --body "Detailed description of changes"
```

**Note:** All changes should go through pull requests for code review before merging to master.

---



