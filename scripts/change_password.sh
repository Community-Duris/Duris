#!/bin/bash

set -e

if [ $# -lt 2 ]; then
    echo "usage: $0 <account_name> <new_password>"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../.env"

ACCOUNT="$1"
PASSWORD="$2"

# generate bcrypt hash (cost 12, same as mud uses)
HASH=$(mkpasswd -m bcrypt -R 12 "$PASSWORD")

echo "updating password for $ACCOUNT..."
mysql -h"$DB_HOST" -u"$DB_USER" -p"$DB_PASSWD" "$DB_NAME" -e "UPDATE accounts SET password='$HASH' WHERE LOWER(account_name)=LOWER('$ACCOUNT');"

echo "done."
