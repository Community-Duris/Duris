#!/bin/bash

# backup script - uses mysqldump for db mode, pfile copy for legacy mode

# load .env if exists
if [ -f .env ]; then
  set -a
  source .env
  set +a
fi

DATESTR_FULL=`date +%C%y.%m.%d-%H.%M.%S`
DATESTR=`date +%s`
# keep backups for 2 days
OLDDATES=`expr $DATESTR - 2 \* 24 \* 60 \* 60`

# check if using db mode
if [ "$REDIS" = "true" ] || [ "$REDIS" = "1" ]; then
  # db mode - use mysqldump
  echo "DB mode enabled, using mysqldump..."

  BACKUP_DIR="db/Backup"
  mkdir -p $BACKUP_DIR

  # cleanup old backups
  for f in $BACKUP_DIR/*.sql.gz; do
    [ -f "$f" ] || continue
    fname=$(basename "$f" .sql.gz)
    if [[ $fname < $OLDDATES ]]; then
      echo "Removing old backup $f"
      rm -f "$f"
    fi
  done

  # dump db
  BACKUP_FILE="$BACKUP_DIR/$DATESTR.sql.gz"
  echo "Creating backup: $BACKUP_FILE = $DATESTR_FULL"
  mysqldump -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASSWD" "$DB_NAME" \
    --single-transaction --quick | gzip > "$BACKUP_FILE"

  if [ $? -eq 0 ]; then
    echo "Backup complete: $(du -h $BACKUP_FILE | cut -f1)"
  else
    echo "Backup failed!"
  fi

else
  # legacy pfile mode - original behavior
  echo "Pfile mode, backing up flat files..."

  FILENAMES=`ls Players/Backup/ 2>/dev/null`
  FILENAMES_LENGTH=${#FILENAMES}

  if [[ `expr $FILENAMES_LENGTH \> 0` ]]; then
    for WORD in $FILENAMES; do
      if [[ $WORD < $OLDDATES ]]; then
        DATE=`date -d @${WORD}`
        echo "Removing old backup Players/Backup/$WORD = $DATE"
        rm -r Players/Backup/$WORD
      fi
    done
  fi

  echo "Creating backup directory: Players/Backup/$DATESTR = $DATESTR_FULL"
  mkdir -p Players/Backup/$DATESTR

  for letter in {a..z}; do
    mkdir -p Players/Backup/$DATESTR/$letter
    cp Players/$letter/* Players/Backup/$DATESTR/$letter 2>/dev/null
  done

  mkdir Players/Backup/$DATESTR/Ships
    cp Ships/* Players/Backup/$DATESTR/Ships 2>/dev/null
  mkdir Players/Backup/$DATESTR/Corpses
    cp Players/Corpses/* Players/Backup/$DATESTR/Corpses 2>/dev/null
  mkdir Players/Backup/$DATESTR/Justice
    cp Players/Justice/* Players/Backup/$DATESTR/Justice 2>/dev/null
  mkdir Players/Backup/$DATESTR/SavedItems
    cp Players/SavedItems/* Players/Backup/$DATESTR/SavedItems 2>/dev/null
  mkdir Players/Backup/$DATESTR/Kingdoms
    cp Players/Kingdoms/* Players/Backup/$DATESTR/Kingdoms 2>/dev/null
  mkdir Players/Backup/$DATESTR/ShopKeepers
    cp Players/ShopKeepers/* Players/Backup/$DATESTR/ShopKeepers 2>/dev/null
  mkdir Players/Backup/$DATESTR/Assocs
    cp Players/Assocs/* Players/Backup/$DATESTR/Assocs 2>/dev/null
  mkdir Players/Backup/$DATESTR/Shapechange
    cp -r Players/Shapechange/* Players/Backup/$DATESTR/Shapechange 2>/dev/null
  mkdir Players/Backup/$DATESTR/Tradeskills
    cp -r Players/Tradeskills/* Players/Backup/$DATESTR/Tradeskills 2>/dev/null
  mkdir Players/Backup/$DATESTR/House
    cp -r Players/House/* Players/Backup/$DATESTR/House 2>/dev/null
fi
