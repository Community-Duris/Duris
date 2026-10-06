#!/usr/bin/env python3
"""Retained, dated identity effort and XP publication; optional disposable SQL."""
from __future__ import annotations

from copy import deepcopy
from datetime import date
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/telemetry'))
import identity_publication as publication
import identity_history as identity
import incident
import test_telemetry_observations as source
import test_telemetry_identity_history as review
from rollup_definitions import (RollupTarget, UTC_UNKNOWN, ROLLUP_QUALITY_INCIDENT_GAP,
    report_definition, report_catalog)
from rollup_engine import build_page_contributions, BoundsExceeded

TARGET = RollupTarget(3, 1, 8, 7)


def raw_interval(**updates):
    row = source._review.ReviewSemanticsTest.interval()
    row.update(updates)
    return row


def owned(**updates):
    values = dict(at_monotonic_usec=0, at_utc_usec=0, occurrence_utc_usec=0, ownership_account_token=100)
    values.update(updates)
    return source.ownership(**values)


def current_registry(rows=None, **updates):
    return identity.Registry.from_packet(review.packet(rows, **updates))


def coverage(rows=()):
    packet = incident.template(2)
    packet.update(environment_id=8, season_id=7, reviewer_token='a'*64, review_evidence_digest='b'*64,
        reviewed_from_utc_usec=0, reviewed_through_utc_usec=1_000, incidents=list(rows))
    meta, details = incident.validate_packet(packet)
    details = [dict(row, occurrence_relation=1) for row in details]
    summary = incident.publication_summary(TARGET.scope_tuple, meta, details)
    return incident.public_coverage(summary, details, registry_schema_version=2, occurrence_window=(0, 1_000))


def run_publication(rows, registry='default', reviewed=None, **kwargs):
    contribution = build_page_contributions(rows, TARGET, max_page_bytes=8*1024*1024)
    inputs = contribution.identity_inputs
    header = publication.initial_header(TARGET.scope_tuple, 0)
    for row in inputs:
        header['source_digest'] = publication.advance_digest(header['source_digest'], row)
    header.update(source_fact_count=len(inputs), input_watermark=contribution.cursor,
        quality_flags=contribution.state_quality_flags)
    return publication.build_publication(TARGET.scope_tuple, header, inputs,
        current_registry() if registry == 'default' else registry, coverage() if reviewed is None else reviewed, **kwargs)


def effort(rows, basis, token, *, partition=0, category=2):
    return next(row for row in rows if row['basis'] == publication.BASES[basis] and
        row['identity_token'] == token and row['partition_kind'] == partition and row['category'] == category)


class IdentityPublicationTest(unittest.TestCase):
    def test_definition_catalog_matches_its_actual_storage_contract(self):
        rows = [owned(), source.progression(ingest_id=2, record_seq=2)]
        page = build_page_contributions(rows, TARGET)
        self.assertEqual(page.observations.output_fanout, 0)
        self.assertEqual(len(page.identity_inputs), 2)
        self.assertEqual({report['name'] for report in report_catalog(3)},
            {'session_playtime', 'cohort_activity', 'identity_effort', 'portfolio_progression'})
        for name in ('progression_observations', 'level_observations', 'encounter_observations',
                     'encounter_participants', 'combat_contributions'):
            self.assertEqual(report_definition(name, 2).definition_version, 2)
            with self.assertRaises(ValueError):
                report_definition(name, 3)

    def test_six_overlapping_characters_publish_one_controller_clock(self):
        rows = []
        associations = []
        for number in range(6):
            subject, account = 9001+number, 100+number
            seq = 1+number*2
            rows.extend([owned(ingest_id=seq, record_seq=seq, subject_id=subject, pid=42+number,
                session_seq=number+1, ownership_account_token=account),
                raw_interval(ingest_id=seq+1, record_seq=seq+1, subject_id=subject, pid=42+number, session_seq=number+1)])
            associations.append(review.association(number+1, account))
        header, efforts, xp = run_publication(rows, current_registry(associations))
        controller = effort(efforts, 'controller', 500)
        self.assertEqual((controller['character_usec'], controller['union_usec'], controller['distinct_characters'],
            controller['distinct_accounts']), (600, 100, 6, 6))
        self.assertEqual(header['observed_character_usec'], 600)
        self.assertEqual(header['confirmed_controller_character_usec'], 600)
        self.assertEqual(xp, [])
        self.assertEqual(sum(row['character_usec'] for row in efforts if row['basis'] == 1 and row['partition_kind'] == 0), 600)

    def test_unknown_review_and_missing_ownership_are_explicit(self):
        header, efforts, _ = run_publication([owned(), raw_interval(ingest_id=2, record_seq=2)], registry=None)
        self.assertEqual(effort(efforts, 'account', 100)['union_usec'], 100)
        self.assertIsNone(effort(efforts, 'unknown_controller', 0)['union_usec'])
        self.assertEqual(header['unlinked_controller_character_usec'], 100)
        header, efforts, _ = run_publication([raw_interval()])
        self.assertEqual(header['unknown_account_character_usec'], 100)
        self.assertIsNone(effort(efforts, 'unknown_account', 0)['covered_union_usec'])
        self.assertEqual(effort(efforts, 'character', 9001)['union_usec'], 100)

    def test_transfers_and_review_clips_preserve_measured_effort(self):
        rows = [owned(), owned(ingest_id=2, record_seq=2, at_monotonic_usec=50, at_utc_usec=50,
            occurrence_utc_usec=50, ownership_account_token=101, ownership_source=4), raw_interval(ingest_id=3, record_seq=3)]
        registry = current_registry([review.association(), review.association(2, 101, controller_token=501)])
        header, efforts, _ = run_publication(rows, registry)
        self.assertEqual(effort(efforts, 'controller', 500)['character_usec'], 50)
        self.assertEqual(effort(efforts, 'controller', 501)['character_usec'], 50)
        self.assertEqual(header['observed_character_usec'], 100)
        registry = current_registry(reviewed_from_utc_usec=20, reviewed_through_utc_usec=80)
        header, efforts, _ = run_publication([owned(), raw_interval(ingest_id=2, record_seq=2)], registry)
        self.assertEqual(header['confirmed_controller_character_usec'], 60)
        self.assertEqual(header['unlinked_controller_character_usec'], 40)
        self.assertEqual(effort(efforts, 'account', 100)['character_usec'], 100)

    def test_original_session_does_not_import_copyover_producer_clock(self):
        rows = [owned(), raw_interval(ingest_id=2, record_seq=2, boot_id=101, process_id=201)]
        header, efforts, _ = run_publication(rows)
        self.assertEqual(header['unknown_account_character_usec'], 100)
        rows.insert(1, owned(ingest_id=2, record_seq=2, boot_id=101, process_id=201,
            connection_boot_id=101, connection_process_id=201, ownership_source=3))
        rows[-1].update(ingest_id=3, record_seq=3)
        header, efforts, _ = run_publication(rows)
        self.assertEqual(header['owned_character_usec'], 100)

    def test_ownership_quality_and_clock_conflict_retain_character_clock(self):
        for updates in ({'quality_flags': 1 << 7}, {'at_utc_usec': UTC_UNKNOWN, 'occurrence_utc_usec': UTC_UNKNOWN},
                        {'at_utc_usec': 1, 'occurrence_utc_usec': 1}):
            rows = [owned(**updates), raw_interval(ingest_id=2, record_seq=2)]
            header, efforts, _ = run_publication(rows)
            self.assertEqual(effort(efforts, 'character', 9001)['union_usec'], 100)
            self.assertIsNone(effort(efforts, 'account', 100)['union_usec'])
            self.assertEqual(header['confirmed_controller_character_usec'], 0)

    def test_incident_loss_requires_actual_recovery_anchor_and_unknown_end_stays_unknown(self):
        packet = incident.template(2)
        loss = dict(packet['incidents'][0], producer_boot_id=100, producer_process_id=200,
            start_utc_usec=20, end_utc_usec=40, record_kind_mask=1 << 9, evidence_digest='d'*64)
        rows = [owned(), owned(ingest_id=2, record_seq=2, at_monotonic_usec=60, at_utc_usec=60,
            occurrence_utc_usec=60, ownership_source=2), raw_interval(ingest_id=3, record_seq=3)]
        header, efforts, _ = run_publication(rows, reviewed=coverage([loss]))
        self.assertEqual(header['unknown_account_character_usec'], 40)
        self.assertEqual(header['incident_affected_character_usec'], 40)
        self.assertEqual(effort(efforts, 'controller', 500)['covered_union_usec'], 60)
        self.assertTrue(header['quality_flags'] & ROLLUP_QUALITY_INCIDENT_GAP)
        header, _, _ = run_publication(rows, reviewed=coverage([dict(loss, end_utc_usec=None)]))
        self.assertEqual(header['unknown_account_character_usec'], 80)
        header, _, _ = run_publication(rows, reviewed=coverage([dict(loss, status='withdrawn')]))
        self.assertEqual(header['unknown_account_character_usec'], 0)

    def test_xp_uses_point_ownership_and_excludes_level_thresholds(self):
        rows = [owned(), source.progression(ingest_id=2, record_seq=2, at_monotonic_usec=50,
            at_utc_usec=50, occurrence_utc_usec=50),
            source.progression(ingest_id=3, record_seq=3, progression_kind=2,
                progression_requested_xp=0, progression_computed_xp=0, progression_applied_xp=0,
                progression_before_exp=0, progression_after_exp=0, progression_after_level=21, progression_threshold_xp=999_999)]
        header, efforts, xp = run_publication(rows)
        self.assertEqual(header['progression_count'], 2)
        self.assertEqual(efforts, [])
        amount = next(row for row in xp if row['basis'] == 3 and row['partition_kind'] == 0)
        self.assertEqual(amount['earned_positive_xp'], 50)
        self.assertEqual(amount['applied_xp'], 50)
        self.assertEqual(len(xp), 6)

    def test_conflicting_ownership_clock_cannot_close_an_incident_gap(self):
        loss = dict(incident.template(2)['incidents'][0], producer_boot_id=100,producer_process_id=200,
            start_utc_usec=20,end_utc_usec=40,record_kind_mask=1<<9,evidence_digest='d'*64)
        rows = [owned(),owned(ingest_id=2,record_seq=2,at_monotonic_usec=50,
            at_utc_usec=55,occurrence_utc_usec=55,ownership_source=2),
            raw_interval(ingest_id=3,record_seq=3),source.progression(ingest_id=4,
                record_seq=4,at_monotonic_usec=60,at_utc_usec=60,occurrence_utc_usec=60)]
        header, _, xp=run_publication(rows,reviewed=coverage([loss]))
        self.assertEqual(header['unknown_account_character_usec'],80)
        unknown=next(row for row in xp if row['basis']==4 and row['partition_kind']==0)
        self.assertEqual(unknown['incident_affected_observations'],1)

    def test_daily_and_context_partitions_do_not_add_union_cells(self):
        start = publication.DAY_USEC - 50
        end = start + 100
        rows = [owned(at_monotonic_usec=start, at_utc_usec=start, occurrence_utc_usec=start),
            raw_interval(ingest_id=2, record_seq=2, start_monotonic_usec=start, end_monotonic_usec=end,
                start_utc_usec=start, end_utc_usec=end, occurrence_utc_usec=end)]
        header, efforts, _ = run_publication(rows, current_registry(reviewed_through_utc_usec=end+1, reviewed_at_utc_usec=end+1))
        controller = [row for row in efforts if row['basis'] == 3 and row['partition_kind'] == 0]
        self.assertEqual({row['utc_day'] for row in controller}, {date(1970,1,1), date(1970,1,2)})
        self.assertEqual([row['union_usec'] for row in controller], [50,50])
        metadata = publication.public_header(header, identity.generation_row(TARGET.scope_tuple,
            current_registry(reviewed_through_utc_usec=end+1, reviewed_at_utc_usec=end+1)))
        self.assertFalse(metadata['union_cells_additive'])
        self.assertFalse(metadata['rates_computed'])

    def test_xp_reward_sources_and_statuses_remain_separate(self):
        rows = [owned()]
        cases = ((3,1,1,0,50), (0,2,1,0,-20), (0,3,1,0,10),
                 (11,4,1,0,777), (3,1,2,1,50), (3,1,3,0,0))
        for seq, (source_kind, reason, status, modifiers, amount) in enumerate(cases,2):
            rows.append(source.progression(ingest_id=seq, record_seq=seq,
                progression_source=source_kind, progression_reason=reason,
                progression_observation_status=status, progression_modifier_flags=modifiers,
                progression_requested_xp=amount, progression_computed_xp=amount,
                progression_applied_xp=amount, progression_after_exp=100+amount))
        header, _, xp = run_publication(rows)
        cells = {(row['source'],row['reason'],row['observation_status'],row['modifier_flags']):row
            for row in xp if row['basis']==3 and row['partition_kind']==0}
        self.assertEqual(header['progression_count'],6)
        self.assertEqual(len(cells),6)
        self.assertEqual(cells[(3,1,1,0)]['earned_positive_xp'],50)
        self.assertEqual(cells[(0,2,1,0)]['death_loss_xp'],20)
        self.assertEqual(cells[(0,3,1,0)]['restored_positive_xp'],10)
        self.assertEqual(cells[(11,4,1,0)]['earned_positive_xp'],0)
        self.assertEqual(cells[(11,4,1,0)]['applied_xp'],777)
        self.assertEqual(cells[(3,1,3,0)]['zero_applied_observations'],1)

    def test_empty_publication_does_not_imply_complete_controller_coverage(self):
        header, effort_rows, xp = run_publication([], registry=None)
        self.assertEqual((effort_rows,xp),([],[]))
        metadata = publication.public_header(header,identity.generation_row(TARGET.scope_tuple,None))
        self.assertEqual(metadata['identity']['status'],'published_unknown_identity')
        self.assertFalse(metadata['identity']['complete_identity_coverage_implied'])
        self.assertFalse(metadata['controller_population_complete_implied'])
        self.assertEqual(metadata['source_fact_count'],0)

    def test_retained_codec_rejects_extra_fields_with_recomputed_digest(self):
        item = build_page_contributions([owned()], TARGET).identity_inputs[0]
        for field,value in (('private_account_name',123),('ownership_account_token',True)):
            packet = json.loads(item['payload'])
            packet['source'][field]=value
            payload = json.dumps(packet,sort_keys=True,separators=(',',':')).encode()
            changed = dict(item,payload=payload,payload_digest=hashlib.sha256(payload).digest())
            with self.subTest(field=field),self.assertRaises(publication.PublicationError):
                publication.decode_input(changed,TARGET.scope_tuple)

    def test_canonical_source_digest_detects_removal_or_corruption(self):
        result = build_page_contributions([owned(), raw_interval(ingest_id=2, record_seq=2)], TARGET)
        inputs = result.identity_inputs
        header = publication.initial_header(TARGET.scope_tuple, 2)
        header['source_fact_count'] = 2
        for row in inputs:
            header['source_digest'] = publication.advance_digest(header['source_digest'], row)
        for changed in (inputs[:1], [dict(inputs[0], payload_digest=b'x'*32), inputs[1]],
                        [inputs[1], inputs[0]], [dict(inputs[0], subject_id=9002), inputs[1]]):
            with self.subTest(changed=len(changed)), self.assertRaises(publication.PublicationError):
                publication.build_publication(TARGET.scope_tuple, header, changed, current_registry(), coverage())

    def test_overlap_capacity_and_deadline_refuse_whole_publication(self):
        rows = [owned(), raw_interval(ingest_id=2, record_seq=2), raw_interval(ingest_id=3, record_seq=3)]
        with self.assertRaises(identity.IdentityError):
            run_publication(rows)
        with self.assertRaises(publication.PublicationError):
            run_publication([owned(), raw_interval(ingest_id=2, record_seq=2)], max_output_rows=1)
        def expired():
            raise BoundsExceeded('injected deadline')
        with self.assertRaises(BoundsExceeded):
            run_publication([owned()], check_deadline=expired)


def sql_qualification():
    import pymysql
    from db_access import (AmbiguousCommit, ConnectionSettings, DatabaseAccessError, GenerationConflict,
        PyMySQLConnectionFactory, PyMySQLRollupDatabase, RAW_COLUMNS)
    from rollup_engine import RollupEngine, RollupBounds, SemanticError
    from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
    from test_telemetry_incidents import runtime_fingerprint
    if os.environ.get('TELEMETRY_REPOSITORY_DISPOSABLE') != '1':
        raise RuntimeError('explicit disposable qualification required')
    environment, command, name = prepare_sql_fixture()
    admin = None
    databases, users = [], []
    try:
        fresh_fingerprint = runtime_fingerprint(environment)
        admin = pymysql.connect(host='127.0.0.1', port=int(environment['DB_PORT']), user=environment['DB_USER'],
            password=environment['DB_PASSWD'], database=name, autocommit=True,
            cursorclass=pymysql.cursors.DictCursor, connect_timeout=3, read_timeout=10, write_timeout=10)
        def query(statement, values=()):
            with admin.cursor() as cursor:
                cursor.execute(statement, values)
                return list(cursor.fetchall()) if cursor.description else []
        token = hashlib.sha256(name.encode()).hexdigest()[:10]
        password = 'synthetic-identity-publication-' + token
        base_tables = ('telemetry_rollup_state','telemetry_rollup_session','telemetry_cohort_day',
            'telemetry_cohort_member','telemetry_player_day')
        observation_tables = ('telemetry_rollup_progression_day','telemetry_rollup_level_event',
            'telemetry_rollup_encounter','telemetry_rollup_encounter_participant','telemetry_rollup_combat_actor')
        public_tables = ('telemetry_rollup_incident_coverage','telemetry_rollup_incident',
            'telemetry_rollup_identity_coverage','telemetry_rollup_identity_effort','telemetry_rollup_portfolio_xp')
        grants = {
            'review': {table:'SELECT,INSERT' for table in ('telemetry_identity_registry','telemetry_identity_association',
                'telemetry_incident_registry_v2','telemetry_incident_v2')},
            'rollup': {table:'SELECT,INSERT,UPDATE' for table in (*base_tables,*observation_tables)},
            'report': {table:'SELECT' for table in (*base_tables,*observation_tables,*public_tables,'telemetry_generation_identity')},
            'writer': {'telemetry_interval':'SELECT,INSERT'},
        }
        grants['review'].update(telemetry_identity_reviewer='SELECT',telemetry_interval='SELECT',
            telemetry_progression_context='SELECT',telemetry_progression_configuration='SELECT')
        grants['rollup'].update({table:'SELECT' for table in ('telemetry_interval','telemetry_progression_context','telemetry_progression_configuration','telemetry_identity_registry',
            'telemetry_identity_association','telemetry_incident_registry','telemetry_incident',
            'telemetry_incident_registry_v2','telemetry_incident_v2')})
        grants['rollup'].update({table:'SELECT,INSERT' for table in (*public_tables,'telemetry_generation_identity','telemetry_identity_input')})
        grants['rollup']['telemetry_rollup_identity_coverage']='SELECT,INSERT,UPDATE'
        for role,tables in grants.items():
            user = 'tip_'+role+'_'+token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s",(user,password)); users.append(user)
            for table,permissions in tables.items():
                query(f"GRANT {permissions} ON `{name}`.`{table}` TO %s@'%%'",(user,))
        review_user = 'tip_review_'+token
        query(f"GRANT SELECT (environment_id,season_id,account_token) ON `{name}`.telemetry_account_token TO %s@'%%'",(review_user,))
        def settings(role):
            return ConnectionSettings(host='127.0.0.1',port=int(environment['DB_PORT']),database=name,
                user='tip_'+role+'_'+token,password=password)
        def adapter(role):
            result = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings(role)))
            databases.append(result)
            return result
        reviewer, rollup, reporter = adapter('review'),adapter('rollup'),adapter('report')
        def report_dict(target, report_name='identity_effort'):
            snapshot = reporter.read_report(target, report_name)
            return dict(coverage=snapshot.coverage.public_dict(), rows=list(snapshot.rows))
        principal = reviewer.connection_factory.connect()
        try:
            with principal.cursor() as cursor:
                cursor.execute('SELECT CURRENT_USER() AS principal'); principal_name=cursor.fetchone()['principal']
        finally:
            reviewer.connection_factory.close(principal)
        p1 = review.packet()
        query('INSERT INTO telemetry_identity_reviewer VALUES (8,7,%s,%s,1)',(principal_name,bytes.fromhex(p1['reviewer_token'])))
        query('INSERT INTO telemetry_account_lifetime VALUES (1001,NULL),(1002,NULL)')
        query('INSERT INTO telemetry_account_token VALUES (8,7,100,1001),(8,7,101,1002)')
        assert reviewer.register_identity_packet(p1)['status']=='registered'
        empty_review = incident.template(2)
        empty_review.update(environment_id=8,season_id=7,reviewer_token='a'*64,review_evidence_digest='b'*64,
            reviewed_from_utc_usec=0,reviewed_through_utc_usec=1000,incidents=[])
        assert reviewer.register_incident_packet(empty_review)['status']=='registered'
        def raw_insert(row):
            row=dict(row)
            row.pop('ingest_id',None)
            row['ingested_utc_usec']=1 if row['occurrence_utc_usec']==UTC_UNKNOWN else row['occurrence_utc_usec']+1
            assert set(row).issubset(RAW_COLUMNS)
            fields=tuple(row)
            query('INSERT INTO telemetry_interval ('+','.join(fields)+') VALUES ('+','.join(['%s']*len(fields))+')',
                tuple(row[field] for field in fields))
        original_rows=[owned(),raw_interval(record_seq=2),source.progression(record_seq=3,
            at_monotonic_usec=50,at_utc_usec=50,occurrence_utc_usec=50)]
        for row in original_rows: raw_insert(row)
        through=int(query('SELECT MAX(ingest_id) AS n FROM telemetry_interval')[0]['n'])
        bounds=RollupBounds(page_size=1,max_runtime_s=30)
        def prepare(generation,version=1,watermark=None):
            target=RollupTarget(3,generation,8,7)
            rollup.reserve_identity_generation(target.scope_tuple,version)
            assert RollupEngine(rollup).run(target,bounds=bounds,through_ingest_id=through if watermark is None else watermark).complete
            return target
        def publish(generation,version=1):
            target=prepare(generation,version)
            rollup.publish_generation(target,bounds=RollupBounds(max_runtime_s=30))
            return target
        target1=publish(1)
        first=report_dict(target1)
        first_xp=report_dict(target1,'portfolio_progression')
        assert first['coverage']['identity_coverage']['identity']['registry_version']==1
        assert first['coverage']['identity_coverage']['identity']['balance_report_published'] is True
        first_controller=next(row for row in first['rows'] if row['basis']=='controller' and row['partition_kind']=='portfolio')
        assert first_controller['identity_token']==500 and first_controller['character_usec']==100 and first_controller['union_usec']==100
        assert next(row for row in first_xp['rows'] if row['basis']=='controller' and row['partition_kind']=='portfolio')['earned_positive_xp']==50
        rollup.publish_generation(target1)
        # Published source windows are immutable even as raw ingestion advances.
        raw_insert(owned(record_seq=4,at_monotonic_usec=50,at_utc_usec=50,occurrence_utc_usec=50,
            ownership_source=4,ownership_account_token=101))
        through=int(query('SELECT MAX(ingest_id) AS n FROM telemetry_interval')[0]['n'])
        try: RollupEngine(rollup).run(target1,bounds=bounds,through_ingest_id=through)
        except GenerationConflict: pass
        else: raise AssertionError('published identity generation admitted new input')
        p2=review.packet([review.association(controller_token=501),review.association(2,101,controller_token=502)],
            registry_version=2,previous_registry_version=1,previous_packet_digest=identity.Registry.from_packet(p1).packet_digest)
        assert reviewer.register_identity_packet(p2)['status']=='registered'
        target2=prepare(2,2)
        # The maintained process CLI publishes and reads through dedicated roles.
        cli_env=dict(os.environ)
        cli_env.update(TELEMETRY_ROLLUP_DB_HOST='127.0.0.1',TELEMETRY_ROLLUP_DB_PORT=str(environment['DB_PORT']),
            TELEMETRY_ROLLUP_DB_DATABASE=name,TELEMETRY_ROLLUP_DB_USER=settings('rollup').user,
            TELEMETRY_ROLLUP_DB_PASSWORD=password)
        target_args=['--definition-version','3','--generation','2','--environment-id','8','--season-id','7']
        subprocess.run([sys.executable,'scripts/telemetry/rollup.py','run',*target_args,'--page-size','1',
            '--through-ingest-id',str(through),'--max-runtime-s','30'],cwd=ROOT,env=cli_env,capture_output=True,check=True,timeout=45)
        subprocess.run([sys.executable,'scripts/telemetry/rollup.py','publish',*target_args,'--max-runtime-s','30'],
            cwd=ROOT,env=cli_env,capture_output=True,check=True,timeout=45)
        cli_env['TELEMETRY_ROLLUP_DB_USER']=settings('report').user
        output=subprocess.run([sys.executable,'scripts/telemetry/rollup.py','report',*target_args,'--name','identity_effort'],
            cwd=ROOT,env=cli_env,capture_output=True,text=True,check=True,timeout=45)
        assert json.loads(output.stdout)['coverage']['identity_coverage']['identity']['registry_version']==2
        changed=report_dict(target2)
        controller_amounts={row['identity_token']:row['character_usec'] for row in changed['rows'] if row['basis']=='controller' and row['partition_kind']=='portfolio'}
        assert controller_amounts=={501:50,502:50}
        retained=report_dict(target1)
        assert retained['rows']==first['rows'] and retained['coverage']['identity_coverage']==first['coverage']['identity_coverage']
        target3=publish(3,None)
        unknown=report_dict(target3)
        assert unknown['coverage']['identity_coverage']['identity']['status']=='published_unknown_identity'
        assert all(row['union_usec'] is None for row in unknown['rows'] if row['basis']=='unknown_controller')
        # A closed loss still needs an actual same-producer ownership anchor.
        loss=dict(incident.template(2)['incidents'][0],producer_boot_id=100,producer_process_id=200,
            start_utc_usec=20,end_utc_usec=40,record_kind_mask=1<<9,evidence_digest='d'*64)
        incident_v2=dict(empty_review,registry_version=2,previous_registry_version=1,incidents=[loss])
        reviewer.register_incident_packet(incident_v2)
        target4=publish(4,2)
        lost=report_dict(target4)
        assert lost['coverage']['identity_coverage']['unknown_account_character_usec']==30
        assert lost['coverage']['identity_coverage']['incident_affected_character_usec']==30
        reviewer.register_incident_packet(dict(incident_v2,registry_version=3,previous_registry_version=2,
            incidents=[dict(loss,end_utc_usec=None)]))
        target5=publish(5,2)
        tail=report_dict(target5)
        assert tail['coverage']['identity_coverage']['unknown_account_character_usec']==80
        assert report_dict(target4)['coverage']['incident_coverage']['registry_version']==2
        # A later-family failure rolls back outputs, metadata, incident snapshot
        # and publication status together while preserving processed source input.
        target6=prepare(6,2)
        original_execute=rollup._execute
        def fail_xp(statement,parameters=()):
            if statement.startswith('INSERT INTO telemetry_rollup_portfolio_xp'):
                raise DatabaseAccessError('injected later-family publication failure')
            return original_execute(statement,parameters)
        rollup._execute=fail_xp
        try: rollup.publish_generation(target6)
        except DatabaseAccessError: pass
        else: raise AssertionError('partial identity publication acknowledged')
        finally: rollup._execute=original_execute
        for table in ('telemetry_rollup_identity_effort','telemetry_rollup_portfolio_xp','telemetry_rollup_incident_coverage'):
            assert query('SELECT COUNT(*) AS n FROM '+table+' WHERE definition_version=3 AND generation=6')[0]['n']==0
        assert query('SELECT publication_complete FROM telemetry_rollup_identity_coverage WHERE generation=6')[0]['publication_complete']==0
        rollup.publish_generation(target6)
        target7=prepare(7,2)
        original_commit=rollup._commit
        def committed_lost_reply():
            original_commit()
            rollup._commit=original_commit
            raise AmbiguousCommit('injected lost reply after real commit')
        rollup._commit=committed_lost_reply
        rollup.publish_generation(target7)
        assert rollup._connection is not None
        rollup.publish_generation(target7)
        assert reporter.read_report(target7,'identity_effort').coverage.identity_coverage['source_fact_count']==4
        # Missing retained input and tiny reservations refuse complete publication.
        target8=prepare(8,2)
        saved=query('SELECT * FROM telemetry_identity_input WHERE generation=8 ORDER BY ingest_id LIMIT 1')[0]
        query('DELETE FROM telemetry_identity_input WHERE generation=8 AND ingest_id=%s',(saved['ingest_id'],))
        try: rollup.publish_generation(target8)
        except SemanticError: pass
        else: raise AssertionError('missing retained source published as complete')
        fields=tuple(saved)
        query('INSERT INTO telemetry_identity_input ('+','.join(fields)+') VALUES ('+','.join(['%s']*len(fields))+')',tuple(saved[field] for field in fields))
        seen=[]
        original_execute=rollup._execute
        def reserved_read(statement,parameters=()):
            seen.append(statement)
            return original_execute(statement,parameters)
        rollup._execute=reserved_read
        try: rollup.publish_generation(target8,bounds=RollupBounds(max_total_bytes=131072,max_page_bytes=65536))
        except BoundsExceeded: pass
        else: raise AssertionError('unreserved identity inputs fetched')
        finally: rollup._execute=original_execute
        assert not any('FROM telemetry_identity_input' in statement for statement in seen)
        rollup.publish_generation(target8)
        for role,statements in {
            'report':('SELECT * FROM telemetry_identity_input LIMIT 0','SELECT * FROM telemetry_identity_association LIMIT 0',
                'SELECT * FROM telemetry_account_lifetime LIMIT 0','UPDATE telemetry_rollup_identity_effort SET character_usec=0 WHERE 0'),
            'writer':('SELECT * FROM telemetry_identity_input LIMIT 0','SELECT * FROM telemetry_generation_identity LIMIT 0'),
            'review':('SELECT * FROM telemetry_identity_input LIMIT 0','INSERT INTO telemetry_rollup_identity_effort SELECT * FROM telemetry_rollup_identity_effort WHERE 0'),
            'rollup':('SELECT * FROM telemetry_account_lifetime LIMIT 0','UPDATE telemetry_identity_input SET record_kind=1 WHERE 0',
                'DELETE FROM telemetry_rollup_identity_effort WHERE 0'),
        }.items():
            factory=PyMySQLConnectionFactory(settings(role)); connection=factory.connect()
            try:
                with connection.cursor() as cursor:
                    for statement in statements:
                        try: cursor.execute(statement)
                        except pymysql.err.OperationalError as error: assert error.args[0] in (1142,1143)
                        else: raise AssertionError('identity publication role boundary allowed')
            finally: factory.close(connection)
        for statement in ('UPDATE telemetry_rollup_identity_effort SET basis=4,identity_token=123 WHERE generation=8',
            'UPDATE telemetry_rollup_identity_effort SET union_usec=1,unknown_clock_character_usec=1 WHERE generation=8',
            'UPDATE telemetry_rollup_portfolio_xp SET linked_observations=observations+1 WHERE generation=8',
            'UPDATE telemetry_rollup_identity_coverage SET source_fact_count=0 WHERE generation=8'):
            try: query(statement)
            except pymysql.err.MySQLError as error: assert error.args[0] in (3819,4025)
            else: raise AssertionError('invalid identity publication SQL CHECK accepted')
        # A failure after a retained input INSERT rolls back that input, source
        # digest/header and all cursor/activity writes together.
        target9 = RollupTarget(3,9,8,7)
        rollup.reserve_identity_generation(target9.scope_tuple,2)
        original_execute = rollup._execute
        def fail_source_header(statement,parameters=()):
            if statement.startswith('UPDATE telemetry_rollup_identity_coverage SET'):
                raise DatabaseAccessError('injected source-header failure')
            return original_execute(statement,parameters)
        rollup._execute=fail_source_header
        try: RollupEngine(rollup).run(target9,bounds=bounds,through_ingest_id=through)
        except DatabaseAccessError: pass
        else: raise AssertionError('source page partially acknowledged')
        finally: rollup._execute=original_execute
        assert query('SELECT COUNT(*) AS n FROM telemetry_identity_input WHERE generation=9')[0]['n']==0
        state_rows=query('SELECT input_watermark FROM telemetry_rollup_state WHERE definition_version=3 AND generation=9')
        assert not state_rows or state_rows==[dict(input_watermark=0)]
        header_rows=query('SELECT input_watermark,source_fact_count FROM telemetry_rollup_identity_coverage WHERE generation=9')
        assert not header_rows or header_rows==[dict(input_watermark=0,source_fact_count=0)]
        assert RollupEngine(rollup).run(target9,bounds=bounds,through_ingest_id=through).complete
        rollup.publish_generation(target9)
        assert report_dict(target9)['coverage']['identity_coverage']['source_fact_count']==4

        # Reconciliation after a real cursor commit preserves the retained
        # input exactly once, including its unchanged rolling digest.
        target10=RollupTarget(3,10,8,7)
        rollup.reserve_identity_generation(target10.scope_tuple,2)
        original_execute,original_commit=rollup._execute,rollup._commit
        inserted=[False]
        def watch_source_insert(statement,parameters=()):
            result=original_execute(statement,parameters)
            if statement.startswith('INSERT INTO telemetry_identity_input '): inserted[0]=True
            return result
        def source_commit_lost_reply():
            original_commit()
            if inserted[0]:
                rollup._commit=original_commit
                raise AmbiguousCommit('injected lost reply after source cursor commit')
        rollup._execute,rollup._commit=watch_source_insert,source_commit_lost_reply
        try: assert RollupEngine(rollup).run(target10,bounds=bounds,through_ingest_id=through).complete
        finally: rollup._execute,rollup._commit=original_execute,original_commit
        rollup.publish_generation(target10)
        assert report_dict(target10)['coverage']['identity_coverage']['source_fact_count']==4
        assert query('SELECT COUNT(*) AS n FROM telemetry_identity_input WHERE generation=10')[0]['n']==4
        # The same mixed raw window still runs through the sealed earlier
        # definitions without identity fields or changed observation amounts.
        for definition_version in (1,2):
            legacy=RollupTarget(definition_version,1,8,7)
            assert RollupEngine(rollup).run(legacy,bounds=bounds,through_ingest_id=through).complete
            rollup.publish_generation(legacy)
            snapshot=reporter.read_report(legacy,'session_playtime')
            assert snapshot.rows[0]['covered_active_usec']==100
            assert 'identity_coverage' not in snapshot.coverage.public_dict()
            if definition_version==2:
                observed=reporter.read_report(legacy,'progression_observations')
                assert observed.rows[0]['earned_positive_xp']==50
        assert query('SELECT COUNT(*) AS n FROM telemetry_identity_input WHERE definition_version<>3')[0]['n']==0
        # Release all read transactions before verifier DDL takes metadata locks.
        for database in databases: database.close()
        sql=ROOT/'migrations/immutable/0060_telemetry_identity_publication.sql'
        verifier=['bash','migrations/immutable/0060_telemetry_identity_publication.sh']
        before=query('SELECT COUNT(*) AS n FROM telemetry_identity_input')[0]['n']
        subprocess.run(command+[name],input=sql.read_bytes(),env=environment,check=True,timeout=15)
        assert query('SELECT COUNT(*) AS n FROM telemetry_identity_input')[0]['n']==before
        subprocess.run(verifier,cwd=ROOT,env=environment,capture_output=True,check=True,timeout=45)
        drop='DROP CONSTRAINT' if 'mariadb' in environment['TELEMETRY_REPOSITORY_DB_IMAGE'] else 'DROP CHECK'
        constraint='chk_identity_input_kind'
        query('ALTER TABLE telemetry_identity_input '+drop+' '+constraint)
        query('ALTER TABLE telemetry_identity_input ADD CONSTRAINT '+constraint+' CHECK (record_kind IN (1,6,9,10))')
        assert subprocess.run(verifier,cwd=ROOT,env=environment,capture_output=True,timeout=45).returncode!=0
        query('ALTER TABLE telemetry_identity_input '+drop+' '+constraint)
        query('ALTER TABLE telemetry_identity_input ADD CONSTRAINT '+constraint+' CHECK (record_kind IN (1,6,9))')
        subprocess.run(verifier,cwd=ROOT,env=environment,capture_output=True,check=True,timeout=45)
        assert runtime_fingerprint(environment)==fresh_fingerprint
        suffix='mariadb' if 'mariadb' in environment['TELEMETRY_REPOSITORY_DB_IMAGE'] else 'mysql'
        artifact=dict(engine=environment['TELEMETRY_REPOSITORY_DB_IMAGE'],status='passed',
            migration_head=sql.stem,normalized_metadata_fingerprint=fresh_fingerprint,
            apply_checksum=hashlib.sha256(sql.read_bytes()).hexdigest(),
            verify_checksum=hashlib.sha256(sql.with_suffix('.sh').read_bytes()).hexdigest(),
            atomic_identity_publication=True,maintained_cli=True,retained_history=True,
            earlier_definition_compatibility=True)
        (ROOT/f'bin/telemetry-identity-publication-{suffix}.json').write_text(json.dumps(artifact,indent=2)+'\n')
        print(json.dumps(artifact),flush=True)
    finally:
        for database in databases: database.close()
        if admin is not None:
            try:
                with admin.cursor() as cursor:
                    for user in users: cursor.execute("DROP USER IF EXISTS %s@'%%'",(user,))
            finally: admin.close()
        drop_sql_fixture(environment,command,name)


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sql-fixture',action='store_true')
    args=parser.parse_args()
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(IdentityPublicationTest))
    if not result.wasSuccessful(): raise SystemExit(1)
    if args.sql_fixture: sql_qualification()
