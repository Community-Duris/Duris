#!/usr/bin/env python3
"""
Parse help_index file and import all help topics into the database.
Format: Each entry starts with # on its own line, followed by "TITLE" line, then content
"""

import re
import sys
from pathlib import Path

# Repository root, so the script works regardless of the caller's directory.
ROOT = Path(__file__).resolve().parents[1]

def parse_help_index(filename):
    """Parse help_index file into individual help entries."""
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Split by # on its own line
    entries = content.split('\n#\n')

    help_entries = []
    for entry in entries:
        entry = entry.strip()
        if not entry or entry.startswith('last update:'):
            continue

        lines = entry.split('\n')
        if not lines:
            continue

        # First line should be the title in quotes
        title_line = lines[0].strip()

        # Extract title from quotes
        match = re.match(r'^"([^"]+)"', title_line)
        if not match:
            continue

        title = match.group(1).strip()

        # Rest is content
        content_lines = lines[1:]
        content = '\n'.join(content_lines).strip()

        # Remove the === lines at start/end
        content = re.sub(r'^=+\n', '', content)
        content = re.sub(r'\n=+$', '', content)
        content = content.strip()

        if title and content:
            help_entries.append((title, content))

    return help_entries

if __name__ == '__main__':
    entries = parse_help_index(ROOT / 'lib/information/help_index')

    print(f"Parsed {len(entries)} help entries")
    print("\nFirst 5 titles:")
    for i, (title, content) in enumerate(entries[:5]):
        print(f"  {i+1}. '{title}' ({len(content)} bytes)")

    # Look for human
    for title, content in entries:
        if 'human' in title.lower():
            print(f"\nFound: '{title}'")
            print(content[:200])
