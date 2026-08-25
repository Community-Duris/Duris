"""Source contracts for the boot-time log hygiene fixes.

Each assertion below guards a defect that showed up as an error or warning in
logs/log/* on a clean boot.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


# --- logs/log must exist before the game opens its log files -----------------
# logit() uses fopen(); a missing logs/log silently drops every log write.
cycle = (ROOT / "scripts/cycle_mud.sh").read_text()
assert "mkdir -p logs/log" in cycle
# Rotation must not carry off the tracked placeholder that keeps the dir alive.
assert "! -name .gitignore" in cycle
assert (ROOT / "logs/log/.gitignore").is_file()


# --- the donation subscriber must not block the game loop --------------------
# A blocking redisGetReply() on a 100ms-timeout socket stalled every idle pulse
# and showed up as a once-per-second NEVENT SLOW entry in logs/log/status.
redis_c = (ROOT / "src/redis.c").read_text()
check = redis_c.split("void redis_check_donation_messages(void)", 1)[1]
check = check.split("\nvoid ", 1)[0]
assert "redisGetReply(donation_sub_ctx" not in check
assert "poll(&pfd, 1, 0)" in check
assert "redisBufferRead(" in check
assert "redisGetReplyFromReader(" in check
# src/poll.h shadows <poll.h> through the Makefile's -I. include path.
assert "#include <sys/poll.h>" in redis_c


# --- account saves must not emit a NULL pid ----------------------------------
# account_characters.pid is NOT NULL; writing NULL aborted the whole account
# transaction, discarding the accounts and account_ips writes with it.
sql_player = (ROOT / "src/sql_player.c").read_text()
save_chars = sql_player.split("static bool sql_save_account_characters(", 1)[1]
save_chars = save_chars.split("\nstatic ", 1)[0]
assert "if (pid <= 0)" in save_chars
assert "deferring mapping row" in save_chars
assert '"NULL"' not in save_chars


# --- a missing ban file is a normal state, not a failure ---------------------
actwiz = (ROOT / "src/actwiz.c").read_text()
read_ban = actwiz.split("void read_ban_file(void)", 1)[1].split("\nvoid ", 1)[0]
assert "errno != ENOENT" in read_ban
assert "#include <errno.h>" in actwiz


# --- only an owned artifact with a zero timer is worth warning about ---------
artifact = (ROOT / "src/artifact.c").read_text()
artifact_lines = artifact.splitlines()
timer_warnings = [
    n
    for n, line in enumerate(artifact_lines)
    if "WARNING: timer was" in line and "resetting to 10 days" in line
]
assert len(timer_warnings) == 3
for n in timer_warnings:
    guard = artifact_lines[n - 1].strip()
    assert guard in ("if (owned)", "if (new_owned)"), guard


# --- MAX_TRADE must cover the largest trade list shipped in world.shp --------
config_h = (ROOT / "src/config.h").read_text()
max_trade = int(re.search(r"#define MAX_TRADE\s+(\d+)", config_h).group(1))

shop_file = ROOT / "areas/world.shp"
if shop_file.is_file():
    lines = shop_file.read_text(encoding="latin-1").split("\n")
    longest = 0
    i = 0
    while i < len(lines):
        if not (lines[i].startswith("#") and lines[i].rstrip().endswith("~")):
            i += 1
            continue
        i += 1
        if i < len(lines) and lines[i].strip() == "N":
            i += 1
        while i < len(lines) and lines[i].strip() != "0":  # produce list
            i += 1
        i += 3  # terminator, buy %, sell %
        traded = 0
        while i < len(lines) and lines[i].strip() != "0":  # trade type list
            traded += 1
            i += 1
        longest = max(longest, traded)
    assert max_trade >= longest, f"MAX_TRADE {max_trade} < world.shp needs {longest}"

# --- the rose flavour event must not hand out a NULL object ------------------
# Object vnum 6107 is not in the world, so read_object() returns NULL and
# obj_to_char() logged "no obj: mob" on every occurrence.
handler = (ROOT / "src/handler.c").read_text()
rose = handler.split("P_obj flow = read_object(6107, VIRTUAL);", 1)[1][:400]
assert "if (flow)" in rose
assert rose.index("if (flow)") < rose.index("obj_to_char(flow, t_ch);")
assert rose.index("if (flow)") < rose.index("do_give(t_ch, text, CMD_GIVE);")


print("boot log hygiene contracts passed")
