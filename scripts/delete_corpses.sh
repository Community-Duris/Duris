#!/bin/bash

# delete all corpses from database and redis
# use with caution - this is destructive

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# look for .env in project root or current dir
if [ -f "$PROJECT_ROOT/.env" ]; then
    source "$PROJECT_ROOT/.env"
elif [ -f ".env" ]; then
    source ".env"
else
    echo "error: .env file not found"
    echo "looked in: $PROJECT_ROOT/.env and ./.env"
    echo ""
    echo "create .env with:"
    echo "  DB_USER=your_user"
    echo "  DB_PASSWD=your_password"
    echo "  DB_NAME=your_database"
    echo "  DB_HOST=127.0.0.1"
    exit 1
fi

# support both DB_PASS and DB_PASSWD
DB_PASS="${DB_PASS:-$DB_PASSWD}"

# check required credentials
if [ -z "$DB_USER" ] || [ -z "$DB_PASS" ] || [ -z "$DB_NAME" ]; then
    echo "error: missing database credentials in .env"
    echo "required: DB_USER, DB_PASS (or DB_PASSWD), DB_NAME"
    echo "optional: DB_HOST (defaults to 127.0.0.1)"
    exit 1
fi

DB_HOST="${DB_HOST:-127.0.0.1}"

echo "=== corpse cleanup script ==="
echo ""

# check database corpses
echo "checking database ($DB_NAME@$DB_HOST)..."
db_corpse_count=$(mysql -u"$DB_USER" -p"$DB_PASS" -h"$DB_HOST" "$DB_NAME" -N -e "SELECT COUNT(*) FROM corpses;" 2>/dev/null)
db_item_count=$(mysql -u"$DB_USER" -p"$DB_PASS" -h"$DB_HOST" "$DB_NAME" -N -e "SELECT COUNT(*) FROM corpse_items;" 2>/dev/null)

if [ -z "$db_corpse_count" ]; then
    echo "  could not connect to database (user=$DB_USER, db=$DB_NAME)"
    echo "  set DB_USER, DB_PASS, DB_NAME, DB_HOST env vars if needed"
    db_corpse_count=0
    db_item_count=0
else
    echo "  corpses table: $db_corpse_count corpses"
    echo "  corpse_items table: $db_item_count items"

    if [ "$db_corpse_count" -gt 0 ]; then
        echo ""
        echo "  recent corpses:"
        mysql -u"$DB_USER" -p"$DB_PASS" -h"$DB_HOST" "$DB_NAME" -e "SELECT id, player_name, room_vnum, created_at FROM corpses ORDER BY created_at DESC LIMIT 5;" 2>/dev/null
    fi
fi

echo ""

# check redis corpses
echo "checking redis..."
redis_world_state=$(redis-cli GET mud:world_state 2>/dev/null)

if [ -z "$redis_world_state" ]; then
    echo "  no world_state in redis"
    redis_corpse_count=0
else
    redis_corpse_count=$(echo "$redis_world_state" | python3 -c "
import sys,json
try:
    d=json.load(sys.stdin)
    corpses=[o for o in d.get('objs',[]) if o.get('v')==2]
    print(len(corpses))
except:
    print(0)
" 2>/dev/null)
    echo "  world_state corpses: $redis_corpse_count"

    if [ "$redis_corpse_count" -gt 0 ]; then
        echo ""
        echo "  sample corpses (first 5):"
        echo "$redis_world_state" | python3 -c "
import sys,json,datetime
try:
    d=json.load(sys.stdin)
    corpses=[o for o in d.get('objs',[]) if o.get('v')==2]
    for c in corpses[:5]:
        uid = c.get('uid', '?')
        rm = c.get('rm', '?')
        v6 = c.get('v6', 0)
        if v6 > 1000000000:
            ts = datetime.datetime.fromtimestamp(v6).strftime('%Y-%m-%d %H:%M')
        else:
            ts = 'n/a'
        nm = c.get('nm', 'corpse')[:30]
        print(f'    uid={uid} room={rm} time={ts} name={nm}')
except Exception as e:
    print(f'    error: {e}')
" 2>/dev/null
    fi
fi

# check floor_drops
floor_drops=$(redis-cli HLEN mud:floor_drops 2>/dev/null)
echo "  floor_drops entries: ${floor_drops:-0}"

echo ""
echo "=== summary ==="
echo "  database: $db_corpse_count corpses, $db_item_count items"
echo "  redis world_state: $redis_corpse_count corpses"
echo "  redis floor_drops: ${floor_drops:-0} entries"
echo ""

total=$((db_corpse_count + redis_corpse_count))
if [ "$total" -eq 0 ]; then
    echo "nothing to delete."
    exit 0
fi

# ask for confirmation
read -p "delete all corpses? (yes/no): " confirm

if [ "$confirm" != "yes" ]; then
    echo "cancelled."
    exit 0
fi

echo ""
echo "deleting..."

# delete from database
if [ "$db_corpse_count" -gt 0 ]; then
    echo "  clearing database tables..."
    mysql -u"$DB_USER" -p"$DB_PASS" -h"$DB_HOST" "$DB_NAME" -e "
        DELETE FROM corpse_item_affects;
        DELETE FROM corpse_items;
        DELETE FROM corpses;
    " 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "    done"
    else
        echo "    failed"
    fi
fi

# delete from redis world_state
if [ "$redis_corpse_count" -gt 0 ]; then
    echo "  removing corpses from redis world_state..."
    echo "$redis_world_state" | python3 -c "
import sys,json
d=json.load(sys.stdin)
before=len(d.get('objs',[]))
d['objs']=[o for o in d.get('objs',[]) if o.get('v')!=2]
after=len(d['objs'])
print(json.dumps(d))
" 2>/dev/null | redis-cli -x SET mud:world_state > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "    done"
    else
        echo "    failed"
    fi
fi

# clear floor_drops (may contain corpses)
if [ "${floor_drops:-0}" -gt 0 ]; then
    echo "  clearing redis floor_drops..."
    redis-cli DEL mud:floor_drops > /dev/null 2>&1
    echo "    done"
fi

echo ""
echo "cleanup complete. restart the mud to apply changes."
