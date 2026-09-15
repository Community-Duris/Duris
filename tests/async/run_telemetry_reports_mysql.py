#!/usr/bin/env python3
"""Run #269 tests on fresh isolated SQL, using a network-none Python client.

Requires Docker, locally installed PyMySQL, and a named client container with
Python 3. The client is never rebuilt or reconfigured. Only a unique temporary
subdirectory is created in it; the new database sidecar has no host mounts or
published ports. No game environment files are read.
"""
from __future__ import annotations

import argparse
import io
import json
import os
from pathlib import Path
import secrets
import subprocess
import tarfile

import pymysql

ROOT = Path(__file__).resolve().parents[2]
SETUP = r'''
import json, os, pathlib, subprocess, sys, time
import pymysql
cfg=json.load(sys.stdin)
settings=dict(host='127.0.0.1', port=cfg['port'], user='root',
              password=cfg['admin'], connect_timeout=2, autocommit=True)
for attempt in range(90):
    try:
        connection=pymysql.connect(**settings)
        break
    except pymysql.err.OperationalError:
        time.sleep(1)
else:
    raise RuntimeError('isolated report database readiness timed out')
with connection, connection.cursor() as cursor:
    cursor.execute('CREATE DATABASE '+cfg['database'])
    cursor.execute('USE '+cfg['database'])
    for name in ('0014_telemetry_storage.sql','0017_telemetry_rollup_support.sql'):
        sql=(pathlib.Path('migrations/immutable')/name).read_text()
        sql='\n'.join(line for line in sql.splitlines() if not line.lstrip().startswith('--'))
        for statement in sql.split(';'):
            if statement.strip():
                cursor.execute(statement)
    cursor.execute('CREATE USER %s IDENTIFIED BY %s', ('duris269_report', cfg['report']))
    for table in ('telemetry_rollup_state','telemetry_rollup_session',
                  'telemetry_cohort_day','telemetry_cohort_member'):
        cursor.execute('GRANT SELECT ON '+cfg['database']+'.'+table+' TO %s', ('duris269_report',))
    cursor.execute('SELECT VERSION()')
    print('Isolated report database:', cursor.fetchone()[0], flush=True)
env=os.environ.copy()
env.update(TELEMETRY_REPORT_TEST='1', REPORT_TEST_DATABASE=cfg['database'],
           REPORT_TEST_HOST='127.0.0.1', REPORT_TEST_PORT=str(cfg['port']),
           REPORT_TEST_ADMIN_USER='root', REPORT_TEST_ADMIN_PASSWORD=cfg['admin'],
           REPORT_TEST_REPORT_USER='duris269_report', REPORT_TEST_REPORT_PASSWORD=cfg['report'])
raise SystemExit(subprocess.run([sys.executable,'tests/async/telemetry_reports_mysql.py'],env=env).returncode)
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client-container', required=True)
    parser.add_argument('--image', choices=('mariadb:10.11', 'mysql:8.0'), default='mariadb:10.11')
    parser.add_argument('--port', type=int, default=3309)
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error('port must be unprivileged and valid')
    client = json.loads(subprocess.check_output(['docker','inspect',args.client_container]))[0]
    if client['HostConfig']['NetworkMode'] != 'none' or client['Mounts']:
        parser.error('client must have network=none and no mounts')
    subprocess.run(['docker','exec',args.client_container,'python3','-c',
                    'import socket; s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1); s.bind(("127.0.0.1",'+str(args.port)+')); s.close()'], check=True)
    token = secrets.token_hex(6)
    directory = '/tmp/duris269-report-' + token
    name = 'duris-269-report-' + token
    admin, report = secrets.token_hex(24), secrets.token_hex(24)
    config = dict(port=args.port, admin=admin, report=report, database='duris_269_'+token+'_test')
    archive = io.BytesIO()
    with tarfile.open(fileobj=archive, mode='w') as tar:
        for folder in ('scripts/telemetry','tests/async'):
            for path in sorted((ROOT/folder).glob('*.py')):
                tar.add(path, arcname=str(path.relative_to(ROOT)))
        for name_sql in ('0014_telemetry_storage.sql','0017_telemetry_rollup_support.sql'):
            path = ROOT/'migrations/immutable'/name_sql
            tar.add(path, arcname=str(path.relative_to(ROOT)))
        tar.add(Path(pymysql.__file__).parent, arcname='.deps/pymysql',
                filter=lambda info: None if '__pycache__' in info.name else info)
    subprocess.run(['docker','exec',args.client_container,'mkdir','-m','700',directory],check=True)
    container = None
    try:
        subprocess.run(['docker','exec','-i',args.client_container,'tar','xf','-','-C',directory],
                       input=archive.getvalue(),check=True)
        env = os.environ.copy()
        variable = 'MARIADB_ROOT_PASSWORD' if args.image.startswith('mariadb:') else 'MYSQL_ROOT_PASSWORD'
        env[variable] = admin
        command = ['docker','run','-d','--name',name,'--label','hermes.task=duris-269-reports',
                   '--restart=no','--network','container:'+client['Id'],
                   '--tmpfs','/var/lib/mysql:rw','-e',variable]
        if args.image.startswith('mysql:'):
            command += ['-e','MYSQL_ROOT_HOST=%']
        command += [args.image,'--port='+str(args.port),'--event-scheduler=OFF']
        if args.image.startswith('mysql:'):
            command += ['--default-authentication-plugin=mysql_native_password']
        container = subprocess.check_output(command,env=env,text=True).strip()
        result = subprocess.run(['docker','exec','-i','-w',directory,'-e',
                                 'PYTHONPATH='+directory+'/.deps',args.client_container,
                                 'python3','-c',SETUP],input=json.dumps(config),text=True,timeout=240)
        raise SystemExit(result.returncode)
    finally:
        if container:
            info=json.loads(subprocess.check_output(['docker','inspect',container]))[0]
            if info['Config']['Labels'].get('hermes.task') != 'duris-269-reports':
                raise RuntimeError('refusing to remove an unowned database container')
            subprocess.run(['docker','rm','-f',container],check=True,stdout=subprocess.DEVNULL)
        subprocess.run(['docker','exec',args.client_container,'python3','-c',
                        'import shutil; shutil.rmtree('+repr(directory)+')'],check=True)


if __name__ == '__main__':
    main()
