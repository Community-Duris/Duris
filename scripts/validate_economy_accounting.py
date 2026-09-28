#!/usr/bin/env python3
"""Validate economy accounting contracts and report incomplete writer coverage."""

import argparse
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = Path('docs/persistence/economy_accounting')


class ContractError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise ContractError(message)


def integer(value, label, minimum=-(2**63), maximum=2**63-1):
    require(type(value) is int and minimum <= value <= maximum, label)
    return value


def vector(value):
    require(isinstance(value, list) and len(value) == 4, 'four denominations required')
    return [integer(n, 'denomination overflow') for n in value]


def copper(value):
    result = sum(n * unit for n, unit in zip(vector(value), [1, 10, 100, 1000]))
    return integer(result, 'copper overflow')


def unique(rows, key, label):
    values = [row[key] for row in rows]
    require(len(set(values)) == len(values), f'duplicate {label}')


def validate_registry(registry):
    require(registry['schema_version'] == 1, 'unsupported registry version')
    require(registry['denomination_copper'] == [1, 10, 100, 1000], 'denomination units changed')
    for section in ('account_kinds', 'reasons'):
        unique(registry[section], 'id', section)
        unique(registry[section], 'number', section + ' numeric IDs')
        for row in registry[section]:
            integer(row['number'], 'registry ID out of bounds', 1, 65535)
    kinds = {row['id'] for row in registry['account_kinds']}
    for reason in registry['reasons']:
        require(reason['domain'] in registry['domains'], 'unknown reason domain')
        require(set(reason['account_kinds']) <= kinds, 'unknown account kind in reason')
    require(registry['limits']['intent_bytes'] <= 8192, 'intent exceeds command budget')
    require(52 + 40 * 3003 + 393216 + 4 + registry['limits']['intent_bytes'] <= 524288,
            'accounting extension reduces existing command capacity')


def topology(custody):
    for uid, state in custody.items():
        integer(int(uid), 'invalid item UID', 1, 2**64-1)
        require(state['kind'] in {'player','room','container','corpse','locker','auction','shopkeeper','collector','pet','system'},
                'unknown custody owner')
        integer(state['identity'], 'invalid custody identity', 1, 2**64-1)
        integer(state.get('context', 0), 'invalid custody context', 0, 2**64-1)
        root = str(state['root'])
        require(root in custody, 'missing custody root')
        current = uid
        visited = set()
        while True:
            require(current not in visited, 'cyclic topology')
            visited.add(current)
            entry = custody[current]
            require((entry['kind'], entry['identity'], entry.get('context', 0), entry['root']) ==
                    (state['kind'], state['identity'], state.get('context', 0), state['root']),
                    'inconsistent owner/root')
            if entry['parent'] == 0:
                require(current == root, 'wrong root topology')
                break
            current = str(entry['parent'])
            require(current in custody, 'missing parent')


def validate_fixture(fixture, registry):
    reasons = {row['id']: row for row in registry['reasons']}
    kinds = {row['id']: row for row in registry['account_kinds']}
    require(re.fullmatch('[0-9a-f]{32}', fixture['lineage']) and int(fixture['lineage'],16), 'invalid lineage')
    require(re.fullmatch('[0-9a-f]{32}', fixture['epoch']) and int(fixture['epoch'],16), 'invalid epoch')
    holdings = json.loads(json.dumps(fixture['holdings']))
    keys = set()
    for name, account in holdings.items():
        require(account['kind'] in kinds, 'unknown holding account')
        key = (account['kind'], integer(account['identity'], 'invalid account identity', 1, 2**64-1),
               integer(account.get('context',0), 'invalid account context', 0, 2**64-1))
        require(key not in keys, 'duplicate account identity/alias')
        keys.add(key)
        copper(account['balance'])
        require(all(n >= 0 for n in account['balance']), 'negative opening holding')
    custody = json.loads(json.dumps(fixture['custody']))
    topology(custody)
    receipts, sources = {}, set()
    for operation in fixture['operations']:
        operation_id = operation['operation_id']
        require(re.fullmatch('[0-9a-f]{32}', operation_id) and int(operation_id,16), 'invalid operation ID')
        encoded = json.dumps(operation, sort_keys=True, separators=(',',':'))
        if operation_id in receipts:
            require(receipts[operation_id] == encoded, 'operation payload conflict')
            continue  # Exact replay neither recompiles nor reapplies current state.
        require(operation['reason'] in reasons, 'unknown reason')
        policy = reasons[operation['reason']]
        require(operation['actor'] == policy['actor'], 'unauthorized actor')
        if policy['source_event_required']:
            source = operation['source_event']
            require(isinstance(source,str) and re.fullmatch('[0-9a-f]{32}',source) and int(source,16), 'missing source event')
            source_key = (operation['reason'], source)
            require(source_key not in sources, 'duplicate source event')
            sources.add(source_key)
        if policy.get('original_operation_required'):
            original=operation.get('original_operation_id')
            require(original in receipts, 'missing committed original operation')
        require(len(operation['postings']) <= registry['limits']['realized_postings'], 'too many postings')
        require(len(operation['items']) <= registry['limits']['realized_items'], 'too many item events')
        require(len(operation['children']) <= registry['limits']['child_links'], 'too many child links')
        unique(operation['children'], 'operation_id', 'child operation')
        child_ids = {child['operation_id'] for child in operation['children']}
        require(operation_id not in child_ids, 'self-linked operation')
        for child in operation['children']:
            require(re.fullmatch('[0-9a-f]{32}',child['operation_id']) and int(child['operation_id'],16), 'invalid child ID')
            require(child['parent_id'] == operation_id, 'invalid child parent')
        total = 0
        deltas = {}
        for posting in operation['postings']:
            name = posting['account']
            require(name in holdings, 'unknown posting account')
            account = holdings[name]
            require(account['kind'] in policy['account_kinds'], 'unauthorized counterparty')
            value = copper(posting['delta'])
            require(value != 0 or any(posting['delta']), 'empty posting')
            sign = policy.get('system_signs',{}).get(account['kind'],kinds[account['kind']]['sign'])
            require(sign != 'debit_only' or value < 0, 'wrong issuance sign')
            require(sign != 'credit_only' or value > 0, 'wrong sink sign')
            total += value  # Python's unbounded intermediate models checked wider arithmetic.
            delta = deltas.setdefault(name,[0,0,0,0])
            for i, amount in enumerate(posting['delta']):
                delta[i] += amount
        require(total == 0, 'unbalanced operation')
        for name, delta in deltas.items():
            if kinds[holdings[name]['kind']]['sign'] == 'ordinary':
                after = [integer(a+b,'holding overflow') for a,b in zip(holdings[name]['balance'],delta)]
                require(all(n >= 0 for n in after), 'negative holding')
                copper(after)
                holdings[name]['balance'] = after
        unique(operation['items'], 'event_index', 'item event index')
        for index,event in enumerate(operation['items']):
            require(event['event_index']==index, 'nonconsecutive item event index')
            require(event['operation_id'] in child_ids | {operation_id}, 'unlinked item event')
            uid = str(integer(event['uid'],'invalid event UID',1,2**64-1))
            require(event['action'] in registry['item_actions'], 'unknown item action')
            require(custody.get(uid) == event['before'], 'stale item owner/topology')
            if event['action'] in ('create','admit','baseline'):
                require(event['before'] is None and event['after'] is not None, 'duplicate creation/admission')
                require(operation['reason'] in ('item_create','first_admission','baseline'), 'unauthorized item creation')
            elif event['action'] == 'destroy':
                require(event['before'] is not None and event['after'] is None, 'invalid destruction')
                require(operation['reason'] in ('item_destroy','lifecycle_retirement'), 'unauthorized destruction')
            else:
                require(event['before'] is not None and event['after'] is not None, 'invalid transfer')
                require(event['before'] != event['after'], 'empty item movement')
            if event['after'] is None:
                del custody[uid]
            else:
                custody[uid] = event['after']
        topology(custody)
        receipts[operation_id] = encoded
    return holdings, custody


# A conservative lexical census, not proof of reachability or policy correctness.
# Every hit must be reviewed. Mask comments/strings for C++ calls; retain strings
# only for SQL mutation discovery. Source-site snapshots make changed/new hits visible.
LEXEME = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
PATTERNS = {
    'money_helper': r'\b(?:ADD_MONEY|SUB_MONEY|SUB_BANK|insert_money_pickup|transact)\s*\(',
    'economic_submit': r'\b(?:currency_transaction_submit\w*|coin_transfer_command_build|currency_command_build|item_transfer_command_build|item_creation_grant_submit\w*|item_movement_transaction_submit\w*|shop_trade_transaction_submit|collector_transaction_submit|auction_transaction_submit\w*)\s*\(',
    'coin_assignment': r'GET_(?:BALANCE_)?(?:COPPER|SILVER|GOLD|PLATINUM)\([^;\n]*?\)\s*(?:=(?!=)|[+*/-]=|\+\+|--)',
    'direct_cash_assignment': r'\b(?:cash|bank)\s*\[[^;\n]*?\]\s*(?:=(?!=)|[+*/-]=|\+\+|--)',
    'coin_bulk_mutation': r'\b(?:fill|fill_n|memset|memcpy|memmove|add_coins|difficulty_scale_coins)\s*\([^;\n]*(?:cash|GET_|obj|pile)',
    'item_lifecycle': r'\b(?:read_object|instantiate_object_template|create_money|extract_obj|MakeScrap)\s*\(',
    'item_publication': r'\b(?:obj_to_char(?:_at_end)?|obj_to_obj|obj_to_room|obj_from_char|obj_from_obj|obj_from_room|equip_char|unequip_char)\s*\(',
    'sql_economy': r'\b(?:INSERT(?: IGNORE)? INTO|UPDATE|DELETE FROM)\s+(?:currency_ledger|item_current_owner|item_ownership_ledger|account_banks|auction_money_pickups|auction_item_custody|saved_items)\b',
}


def scan_sources(root):
    found=[]
    for path in sorted((root/'src').rglob('*')):
        if not path.is_file() or path.suffix not in ('.c','.h','.cpp','.cc'):
            continue
        source=path.read_text(encoding='utf-8',errors='replace')
        comments_masked=LEXEME.sub(lambda m: re.sub('[^\n]',' ',m[0]) if m[0].startswith(('/',)) else m[0], source)
        code=LEXEME.sub(lambda m: re.sub('[^\n]',' ',m[0]),source)
        for family,pattern in PATTERNS.items():
            for match in re.finditer(pattern,comments_masked if family=='sql_economy' else code):
                line=source.count('\n',0,match.start())+1
                excerpt=source.splitlines()[line-1].strip()
                found.append(dict(path=path.relative_to(root).as_posix(),line=line,family=family,excerpt=excerpt))
    return found


def validate_inventory(inventory, registry, root, release=False):
    require(inventory['schema_version']==1,'unsupported inventory version')
    unique(inventory['writers'],'id','writer ID')
    reasons={r['id'] for r in registry['reasons']}
    census_sites={(site['path'],site['line'],site['family']) for site in inventory['census']}
    for writer in inventory['writers']:
        require(writer['reason'] in reasons,'unknown writer reason')
        require(bool(writer['owner']),'missing integration owner')
        require(bool(writer['authority_boundary']),'missing authority boundary')
        require(bool(writer['classification']),'missing source/sink classification')
        require(set(writer['backends'])=={'mysql','mariadb','flatfile'},'missing backend coverage entry')
        require(all(b['status'] in {'unverified','qualified','refused','projection'} for b in writer['backends'].values()),'unknown backend status')
        integer(writer['integration_issue'],'invalid slice dependency',475,490)
        require(writer['coverage'] in registry['coverage_states'],'unknown coverage state')
        require((root/writer['path']).is_file(),'missing writer source')
        require(writer['symbol'] in (root/writer['path']).read_text(encoding='utf-8',errors='replace'),'missing source anchor')
        for site in writer.get('sites',[]):
            require(tuple(site) in census_sites,'writer source site missing from census')
        for test in writer['test_candidates']:
            require((root/test).is_file(),'missing test candidate')
        if release:
            require(bool(writer.get('evidence')),'writer has no executable evidence')
            expected={'unsupported':'refused','projection':'projection','enforced':'qualified'}.get(writer['coverage'])
            require(expected is not None,'writer not qualified')
            require(all(b['status']==expected and b.get('evidence') for b in writer['backends'].values()),'backend not qualified')
    current=scan_sources(root)
    require(current==inventory['census'],'economic writer census drift; review new/changed sites')
    if release:
        require(inventory['census_complete'],'writer census not complete')
        require(registry['status']=='frozen','registry contract not frozen')
        mapped={tuple(site) for writer in inventory['writers'] for site in writer.get('sites',[])}
        require(all((s['path'],s['line'],s['family']) in mapped for s in current),'unclassified writer candidate')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=ROOT)
    parser.add_argument('--release',action='store_true')
    args=parser.parse_args()
    folder=args.root/CONTRACT
    registry=json.loads((folder/'registry.json').read_text())
    golden=json.loads((folder/'golden.json').read_text())
    inventory=json.loads((folder/'writers.json').read_text())
    validate_registry(registry)
    require(golden['schema_version']==1,'unsupported golden version')
    unique(golden['fixtures'],'id','fixture ID')
    for fixture in golden['fixtures']:
        validate_fixture(fixture,registry)
    validate_inventory(inventory,registry,args.root,args.release)
    print(f"accounting contracts: {len(golden['fixtures'])} fixtures; {len(inventory['writers'])} writer routes; "
          f"{len(inventory['census'])} candidate sites; release_ready={args.release}")
    if not args.release:
        print('Contract validity only; no runtime coverage, storage integration or release qualification claimed.')


if __name__=='__main__':
    try:
        main()
    except (ContractError,KeyError,TypeError,ValueError) as error:
        print(f'economy accounting contract error: {error}',file=sys.stderr)
        raise SystemExit(1)
