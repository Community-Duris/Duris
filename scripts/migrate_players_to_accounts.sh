#!/bin/bash
#
# scripts/migrate_players_to_accounts.sh
#
# This script adds an existing character to an account.
#
# Usage: ./scripts/migrate_players_to_accounts.sh <character_name> <account_name>
#
# Example: ./scripts/migrate_players_to_accounts.sh ubak resakse
#

set -e  # Exit on error

DURIS_DIR="/home/resakse/Coding/DurisMUD"
PLAYERS_DIR="${DURIS_DIR}/Players"
ACCOUNTS_DIR="${DURIS_DIR}/Accounts"
MAX_CHARS_PER_ACCOUNT=8

# Check arguments
if [ $# -ne 2 ]; then
    echo "Usage: $0 <character_name> <account_name>"
    echo ""
    echo "Example: $0 ubak resakse"
    echo ""
    echo "This script adds an existing character to an account."
    exit 1
fi

CHARACTER_NAME="$1"
ACCOUNT_NAME=$(echo "$2" | tr '[:upper:]' '[:lower:]')

echo "DurisMUD Add Character to Account Script"
echo "========================================="
echo ""
echo "Character: $CHARACTER_NAME"
echo "Account:   $ACCOUNT_NAME"
echo ""

# Function to get first letter of name
get_first_letter() {
    local name=$1
    echo "${name:0:1}" | tr '[:upper:]' '[:lower:]'
}

# Check if character exists
CHAR_FIRST_LETTER=$(get_first_letter "$CHARACTER_NAME")
CHAR_FILE="${PLAYERS_DIR}/${CHAR_FIRST_LETTER}/${CHARACTER_NAME}"

if [ ! -f "$CHAR_FILE" ]; then
    echo "ERROR: Character file not found: $CHAR_FILE"
    exit 1
fi

echo "[OK] Character file found: $CHAR_FILE"

# Check if account exists
ACCT_FIRST_LETTER=$(get_first_letter "$ACCOUNT_NAME")
ACCT_FILE="${ACCOUNTS_DIR}/${ACCT_FIRST_LETTER}/${ACCOUNT_NAME}"

if [ ! -f "$ACCT_FILE" ]; then
    echo "ERROR: Account file not found: $ACCT_FILE"
    echo ""
    echo "Please create the account first or check the account name."
    exit 1
fi

echo "[OK] Account file found: $ACCT_FILE"

# Parse account file to find character count
echo ""
echo "Parsing account file structure..."

LINE_NUM=0
NUM_IPS=0
CURRENT_CHAR_COUNT=0
CHARACTER_EXISTS=0
CHAR_COUNT_LINE=0
INSERT_AFTER_LINE=0

# Read the file to understand structure
# Format:
# Lines 1-5: serial, name, email, password, confirmation
# Line 6: number of IPs
# Lines 7+: For each IP: hostname, ip, count (3 lines per IP)
# After IPs: character count
# For each character: name, then "count last blocked racewar" (2 lines per char)
# After all characters: 10 more fields (blocked, confirmed, confirmation_sent, last, good, evil, flags1-4)
# Final line: ###

while IFS= read -r line; do
    LINE_NUM=$((LINE_NUM + 1))

    # Line 6 is number of IPs
    if [ $LINE_NUM -eq 6 ]; then
        NUM_IPS=$line
        echo "  Number of IPs: $NUM_IPS"
    fi

    # Character count line is: 6 + (NUM_IPS * 3) + 1
    if [ $LINE_NUM -eq $((6 + NUM_IPS * 3 + 1)) ]; then
        CURRENT_CHAR_COUNT=$line
        CHAR_COUNT_LINE=$LINE_NUM
        echo "  Current characters in account: $CURRENT_CHAR_COUNT"

        # Calculate where to insert: after the last character entry
        # Each character has 2 lines, so insert after: CHAR_COUNT_LINE + (CURRENT_CHAR_COUNT * 2)
        INSERT_AFTER_LINE=$((CHAR_COUNT_LINE + CURRENT_CHAR_COUNT * 2))
        echo "  Will insert new character after line: $INSERT_AFTER_LINE"
    fi

    # Check if character already exists
    if [ $CHAR_COUNT_LINE -gt 0 ] && [ $LINE_NUM -gt $CHAR_COUNT_LINE ]; then
        OFFSET=$((LINE_NUM - CHAR_COUNT_LINE))
        # Character names are on odd offset lines (1, 3, 5, ...)
        if [ $((OFFSET % 2)) -eq 1 ]; then
            if [ "$line" = "$CHARACTER_NAME" ]; then
                CHARACTER_EXISTS=1
                echo "  ERROR: Found existing character: $line"
            fi
        fi
    fi
done < "$ACCT_FILE"

# Check if character already exists in account
if [ $CHARACTER_EXISTS -eq 1 ]; then
    echo ""
    echo "ERROR: Character '$CHARACTER_NAME' already exists in account '$ACCOUNT_NAME'"
    exit 1
fi

echo "  Character is unique: OK"

# Check character limit
if [ $CURRENT_CHAR_COUNT -ge $MAX_CHARS_PER_ACCOUNT ]; then
    echo ""
    echo "ERROR: Account already has maximum characters ($MAX_CHARS_PER_ACCOUNT)"
    echo "Cannot add more characters to this account."
    exit 1
fi

echo "  Character count OK: $CURRENT_CHAR_COUNT/$MAX_CHARS_PER_ACCOUNT"

# Create backup
BACKUP_FILE="${ACCT_FILE}.backup.$(date +%Y%m%d_%H%M%S)"
cp "$ACCT_FILE" "$BACKUP_FILE"
echo ""
echo "Created backup: $BACKUP_FILE"

# Now modify the account file
echo ""
echo "Modifying account file..."

TEMP_FILE="${ACCT_FILE}.tmp"
LINE_NUM=0
NEW_CHAR_COUNT=$((CURRENT_CHAR_COUNT + 1))

while IFS= read -r line; do
    LINE_NUM=$((LINE_NUM + 1))

    # Update character count line
    if [ $LINE_NUM -eq $CHAR_COUNT_LINE ]; then
        echo "$NEW_CHAR_COUNT" >> "$TEMP_FILE"
        echo "  Updated character count from $CURRENT_CHAR_COUNT to $NEW_CHAR_COUNT"
    # After the last character entry, insert new character
    elif [ $LINE_NUM -eq $INSERT_AFTER_LINE ]; then
        # First write the current line (last character's stats)
        echo "$line" >> "$TEMP_FILE"
        # Then add new character entry (name + stats)
        echo "$CHARACTER_NAME" >> "$TEMP_FILE"
        echo "0 0 0 0" >> "$TEMP_FILE"
        echo "  Inserted character '$CHARACTER_NAME' after line $INSERT_AFTER_LINE"
    else
        echo "$line" >> "$TEMP_FILE"
    fi
done < "$ACCT_FILE"

# Replace original file with modified version
mv "$TEMP_FILE" "$ACCT_FILE"
chmod 600 "$ACCT_FILE"

echo ""
echo "========================================="
echo "SUCCESS!"
echo "========================================="
echo "Character '$CHARACTER_NAME' has been added to account '$ACCOUNT_NAME'"
echo "New character count: $NEW_CHAR_COUNT/$MAX_CHARS_PER_ACCOUNT"
echo ""
echo "Backup saved to: $BACKUP_FILE"
echo ""
