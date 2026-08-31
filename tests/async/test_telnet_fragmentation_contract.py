from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")
mccp = (ROOT / "src" / "mccp.c").read_text(encoding="utf-8", errors="replace")
nanny = (ROOT / "src" / "nanny.c").read_text(encoding="utf-8", errors="replace")

# Fragmented Telnet commands must remain buffered until the next socket read.
assert "memmove(bp, buf + i, len - i);" in comm
assert "bp += len - i;" in comm

# The parser must not inspect the command or option byte until present.
assert "if (buflen < 2)" in mccp
assert mccp.count("if (buflen < 3)") >= 5

# Re-enabling client echo must be one complete Telnet negotiation command.
echo_on = nanny[nanny.index("void echo_on(P_desc d)") : nanny.index("void echo_off(P_desc d)")]
assert "{ IAC, WONT, TELOPT_ECHO }" in echo_on
assert "write_to_descriptor_binary(d, on_string, sizeof(on_string));" in echo_on
assert "TELOPT_NAOFFD" not in echo_on
assert "TELOPT_NAOCRD" not in echo_on
