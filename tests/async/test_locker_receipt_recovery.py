"""Real process exits at receipt/payment boundaries, on both repositories.

Flatfile runs without services. MariaDB additionally runs when TEST_DB_HOST,
TEST_DB_USER and TEST_DB_PASSWORD name a disposable test server; the runner
creates and drops only its own uniquely named schema, never sourcing .env.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[2]
COMMON = ['src/item/locker_receipt.c', 'src/flatfile/flatfile_store.c',
          'src/economy/currency_command.c', 'src/persistence/critical_command.c',
          'src/world/epic_command.c', 'src/combat/combat_outcome_command.c']
SQL = ['src/item/item_transfer_command.c', 'src/item/item_transfer_repository.c',
       'src/economy/auction_command.c', 'src/economy/auction_repository.c',
       'src/combat/combat_outcome_repository.c', 'src/guild/artifact_guild_command.c',
       'src/guild/artifact_guild_repository.c', 'src/economy/boon_reward_command.c',
       'src/economy/boon_reward_repository.c', 'src/world/zone_touch_command.c',
       'src/world/zone_touch_repository.c', 'src/account/session_audit_command.c',
       'src/account/session_audit_repository.c', 'src/economy/coin_transfer_command.c',
       'src/player/player_snapshot_codec.c', 'src/player/player_load_repository.c',
       'src/player/player_load_topology.c', 'src/persistence/persistence_observability.c',
       'src/persistence/critical_command_repository.c']


def run_backend(temp, mysql=False):
    directory = temp / ('mysql' if mysql else 'flatfile')
    directory.mkdir()
    source = directory / 'harness.cpp'
    service = re.sub(r'^#include.*\n', '', (ROOT / 'src/item/locker_identify.c').read_text(), flags=re.M)
    service = service.replace('locker_receipt_write(directory, value)', 'test_receipt_write(directory, value)')
    source.write_text((ROOT / 'tests/async/locker_receipt_harness.cpp').read_text().replace('// SERVICE_BODY', service))
    flags = shlex.split(subprocess.check_output(['mysql_config', '--cflags'], text=True)) if mysql else ['-D__NO_MYSQL__', '-Isrc/no_mysql']
    libraries = shlex.split(subprocess.check_output(['mysql_config', '--libs'], text=True)) if mysql else []
    sources = SQL if mysql else ['src/flatfile/flatfile_player_domain_repository.c', 'src/flatfile/flatfile_authority_transaction.c']
    binary = directory / 'harness'
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc', *flags,
                    str(source), *COMMON, *sources, *libraries, '-lcrypto', '-pthread', '-o', str(binary)], cwd=ROOT, check=True)
    for purse in ('wallet', 'bank'):
        subprocess.run([str(binary), str(directory / ('normal-'+purse)), 'normal', purse], check=True, timeout=30)
        for crash, exit_code in [('before-payment', 77), ('after-payment', 78), ('after-receipt', 79)]:
            state = str(directory / (crash+'-'+purse))
            result = subprocess.run([str(binary), state, crash, purse], timeout=30)
            assert result.returncode == exit_code, (crash, result.returncode)
            subprocess.run([str(binary), state, 'replay' if crash == 'after-receipt' else 'recover', purse], check=True, timeout=30)
            subprocess.run([str(binary), state, 'replay', purse], check=True, timeout=30)


(ROOT / 'bin/tests').mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='locker-recovery-', dir=ROOT / 'bin/tests') as temporary:
    temp = Path(temporary)
    run_backend(temp)
    if os.getenv('TEST_DB_HOST'):
        database = 'locker_receipt_test_' + uuid.uuid4().hex[:12]
        os.environ['LOCKER_TEST_DATABASE'] = database
        environment = dict(os.environ, MYSQL_PWD=os.environ['TEST_DB_PASSWORD'])
        mysql = ['mysql', '--protocol=tcp', '-h', os.environ['TEST_DB_HOST'], '-u', os.environ['TEST_DB_USER']]
        def sql(script, selected=False):
            subprocess.run(mysql + ([database] if selected else []), input=script, text=True, env=environment, check=True)
        sql('CREATE DATABASE ' + database)
        try:
            sql((ROOT / 'migrations/bootstrap_multithread_safe.sql').read_text(), True)
            for migration in sorted((ROOT / 'migrations/immutable').glob('*.sql')):
                sql(migration.read_text(), True)
            sql((ROOT / 'migrations/currency_ledger.sql').read_text(), True)
            run_backend(temp, mysql=True)
        finally:
            sql('DROP DATABASE ' + database)
    else:
        print('MariaDB receipt recovery skipped: TEST_DB_HOST is not set')
