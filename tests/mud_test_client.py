#!/usr/bin/env python3
"""Simple MUD telnet test client - handles IAC negotiation and strips ANSI."""
import socket
import sys
import time
import re
import select

def strip_ansi(text):
    """Remove ANSI escape sequences."""
    return re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', re.sub(r'\x1b\[[0-9;]*m', '', text))

def strip_iac(data):
    """Remove telnet IAC sequences and return clean text."""
    result = bytearray()
    i = 0
    while i < len(data):
        if data[i] == 0xFF:  # IAC
            if i + 1 < len(data):
                cmd = data[i+1]
                if cmd == 0xFF:  # escaped 0xFF
                    result.append(0xFF)
                    i += 2
                elif cmd in (0xFB, 0xFC, 0xFD, 0xFE):  # WILL/WONT/DO/DONT
                    i += 3  # skip IAC + cmd + option
                elif cmd == 0xFF:
                    result.append(0xFF)
                    i += 2
                else:
                    i += 2  # skip unknown
            else:
                i += 1
        else:
            result.append(data[i])
            i += 1
    return bytes(result)

def recv_all(sock, timeout=3):
    """Receive all available data with timeout."""
    chunks = []
    while True:
        ready, _, _ = select.select([sock], [], [], timeout)
        if not ready:
            break
        try:
            data = sock.recv(8192)
            if not data:
                break
            chunks.append(data)
        except:
            break
    return b''.join(chunks)

def mud_login(host, port, account, password, commands=None, timeout=5):
    """Login to MUD and optionally run commands. Returns output log."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(15)
    sock.connect((host, port))
    
    log = []
    
    # Receive banner
    time.sleep(2)
    data = recv_all(sock, timeout=3)
    clean = strip_iac(data)
    text = strip_ansi(clean.decode('utf-8', errors='replace'))
    log.append(("BANNER", text.strip()[-200:]))
    
    # Send account name
    sock.sendall((account + "\r\n").encode())
    time.sleep(2)
    data = recv_all(sock, timeout=3)
    clean = strip_iac(data)
    text = strip_ansi(clean.decode('utf-8', errors='replace'))
    log.append(("AFTER_ACCOUNT", text.strip()))
    
    # Send password
    sock.sendall((password + "\r\n").encode())
    time.sleep(3)
    data = recv_all(sock, timeout=3)
    clean = strip_iac(data)
    text = strip_ansi(clean.decode('utf-8', errors='replace'))
    log.append(("AFTER_PASSWORD", text.strip()))
    
    # Check for login failure
    last_text = log[-1][1]
    if "Invalid Password" in last_text or "disconnecting" in last_text:
        log.append(("RESULT", "LOGIN FAILED"))
        sock.close()
        return log
    
    # Send additional commands if provided
    if commands:
        for cmd in commands:
            sock.sendall((cmd + "\r\n").encode())
            time.sleep(2)
            data = recv_all(sock, timeout=3)
            clean = strip_iac(data)
            text = strip_ansi(clean.decode('utf-8', errors='replace'))
            log.append((f"CMD:{cmd}", text.strip()))
    
    sock.close()
    return log

if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 4000
    account = sys.argv[3] if len(sys.argv) > 3 else "Xanadin"
    password = sys.argv[4] if len(sys.argv) > 4 else "test1234"
    
    print(f"Connecting to {host}:{port} as {account}...")
    log = mud_login(host, port, account, password)
    
    for label, text in log:
        print(f"\n=== {label} ===")
        # Print last 500 chars to keep it manageable
        if len(text) > 500:
            print(f"...{text[-500:]}")
        else:
            print(text)
