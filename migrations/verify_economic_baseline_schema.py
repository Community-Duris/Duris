#!/usr/bin/env python3
"""Read-only exact baseline retention metadata verification; no checkout .env."""
import argparse
import sys
from verify_economy_accounting_schema import Client, VerificationError, fingerprint as metadata_fingerprint

TABLES = ('economic_baseline_control', 'economic_baseline_witness', 'economic_baseline_reservation')
EXPECTED = {'mysql8': 'f0551ebf630d1e18f4bdec863f239da3d974f783f3acafbc243483b8e67bf3bc', 'mariadb10_11': '778e7d3815bc4c66d2bb13072c9bc6689df9008a332bee02b5fd3c84548e0e3e'}

def fingerprint(client):
    return metadata_fingerprint(client, TABLES, item_reference_index=False)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--print-fingerprint', action='store_true')
    args = parser.parse_args()
    engine, actual = fingerprint(Client())
    if args.print_fingerprint:
        print(engine + ' ' + actual)
    elif actual != EXPECTED[engine]:
        raise VerificationError('baseline retention metadata fingerprint mismatch')
    else:
        print('baseline retention schema verified: 3 InnoDB tables, exact metadata')

if __name__ == '__main__':
    try:
        main()
    except (VerificationError, OSError) as error:
        print('baseline schema verification failed: ' + str(error), file=sys.stderr)
        raise SystemExit(1)
