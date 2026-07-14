#!/usr/bin/env python3
"""Drive MUD tests via ttppctl, capturing and filtering output."""
import subprocess
import time
import re
import sys
import json

def ttpp_cmd(session, command, wait=2):
    """Send a command to the MUD via ttppctl and capture output."""
    # Send the command
    subprocess.run(['ttppctl', 'cmd', session, command], 
                   capture_output=True, timeout=10)
    time.sleep(wait)
    # Capture output
    result = subprocess.run(['ttppctl', 'capture', session, '30'],
                          capture_output=True, text=True, timeout=10)
    # Strip ANSI and get last lines after the command
    output = result.stdout
    # Remove ANSI escape sequences
    output = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', output)
    # Split into lines and find the command echo
    lines = output.split('\n')
    # Find lines after the command
    cmd_idx = -1
    for i, line in enumerate(lines):
        if command in line and i > len(lines) // 2:
            cmd_idx = i
            break
    if cmd_idx >= 0:
        relevant = lines[cmd_idx:]
    else:
        relevant = lines[-20:]
    return '\n'.join(relevant)

def ttpp_send(session, text, wait=2):
    """Send raw text to the MUD."""
    subprocess.run(['ttppctl', 'cmd', session, text],
                   capture_output=True, timeout=10)
    time.sleep(wait)

def ttpp_capture(session, lines=20):
    """Capture and clean MUD output."""
    result = subprocess.run(['ttppctl', 'capture', session, str(lines)],
                          capture_output=True, text=True, timeout=10)
    output = result.stdout
    output = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', output)
    return output

def run_test(name, session, commands, wait=2):
    """Run a series of commands and return the output."""
    print(f"\n{'='*60}")
    print(f"TEST: {name}")
    print(f"{'='*60}")
    for cmd in commands:
        print(f"\n>>> {cmd}")
        output = ttpp_cmd(session, cmd, wait)
        # Print just the last 15 lines of relevant output
        lines = output.split('\n')
        for line in lines[-15:]:
            print(f"  {line}")
    return output

if __name__ == "__main__":
    session = "duris"
    
    # Test 1: Save
    run_test("Force Save", session, ["save"])
    
    # Test 2: Locker commands
    run_test("Locker Access", session, ["locker"])
    
    # Test 3: Auction
    run_test("Auction List", session, ["auction list"])
    
    # Test 4: Ship
    run_test("Ship Status", session, ["ship status"])
    
    # Test 5: Inventory
    run_test("Inventory", session, ["inventory"])
    
    # Test 6: Equipment
    run_test("Equipment", session, ["equipment"])
    
    # Test 7: Look
    run_test("Look", session, ["look"])
    
    # Test 8: Help
    run_test("Help - Locker", session, ["help locker"])
    
    print("\n\nAll basic tests complete.")
