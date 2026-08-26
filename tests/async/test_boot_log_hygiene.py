"""Source contracts for the boot-time log hygiene fixes.

Each assertion below guards a defect that showed up as an error or warning in
logs/log/* on a clean boot.
"""

import re
from pathlib import Path
from contract_text import contains, find, index, split_at

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
assert not contains(check, "redisGetReply(donation_sub_ctx")
assert contains(check, "poll(&pfd, 1, 0)")
assert contains(check, "redisBufferRead(")
assert contains(check, "redisGetReplyFromReader(")
# src/poll.h shadows <poll.h> through the Makefile's -I. include path.
assert contains(redis_c, "#include <sys/poll.h>")


# --- account saves must not emit a NULL pid ----------------------------------
# account_characters.pid is NOT NULL; writing NULL aborted the whole account
# transaction, discarding the accounts and account_ips writes with it.
sql_player = (ROOT / "src/sql_player.c").read_text()
# Anchor on the definition; the bare signature also matches the prototype.
save_chars = split_at(
    sql_player, "static bool sql_save_account_characters(struct acct_entry *acc)\n{", 1
)[1]
save_chars = save_chars.split("\nstatic ", 1)[0]
assert contains(save_chars, "if (pid <= 0)")
assert contains(save_chars, "component=mapping outcome=deferred")
assert not contains(save_chars, '"NULL"')


# --- a missing ban file is a normal state, not a failure ---------------------
actwiz = (ROOT / "src/actwiz.c").read_text()
read_ban = actwiz.split("void read_ban_file(void)", 1)[1].split("\nvoid ", 1)[0]
assert contains(read_ban, "errno != ENOENT")
assert contains(actwiz, "#include <errno.h>")


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
    # clang-format wraps the logit() call, so the ownership guard sits a couple
    # of lines above the message rather than immediately before it.
    window = [line.strip() for line in artifact_lines[max(0, n - 4) : n]]
    assert any(g in window for g in ("if (owned)", "if (new_owned)")), window


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
assert contains(rose, "if (flow)")
assert index(rose, "if (flow)") < index(rose, "obj_to_char(flow, t_ch);")
assert index(rose, "if (flow)") < index(rose, "do_give(t_ch, text, CMD_GIVE);")


# --- the exit log must not report stale errno as a shutdown failure ----------
# LOG_EXIT is also used for normal termination messages.  perror() appended an
# unrelated EAGAIN left by the nonblocking game loop to each of those messages.
utility = (ROOT / "src/utility.c").read_text()
logit = utility.split("void logit(const char *filename, const char *format, ...)", 1)[1]
logit = logit.split("\nvoid ", 1)[0]
assert contains(logit, "fputs(lbuf, stderr)")
assert not contains(logit, "perror(lbuf)")


# --- Heaven's persisted zone number must match its first room vnum -----------
# Zone numbers are defined as first_room_vnum / 100; Heaven begins below 100.
heaven_zone = (ROOT / "areas/zon/heavens.zon").read_text()
assert heaven_zone.startswith("#0\n")


# --- ACT_SPEC is derived from assigned functions -----------------------------
# boot_mobiles() strips an unassigned source bit and sets it when a function is
# present.  Keeping the derived bit out of active area sources prevents dormant
# prototypes from producing configuration warnings when they eventually spawn.
active_area_names = [
    line.split()[0]
    for line in (ROOT / "areas/AREA").read_text().splitlines()
    if line.strip() and not line.lstrip().startswith(("*", "#"))
]
for area_name in active_area_names:
    mob_source = (ROOT / f"areas/mob/{area_name}.mob").read_text()
    headers = list(re.finditer(r"(?m)^#(\d+)\n", mob_source))
    for record_number, header in enumerate(headers):
        vnum = int(header.group(1))
        record_end = (
            headers[record_number + 1].start()
            if record_number + 1 < len(headers)
            else len(mob_source)
        )
        record = mob_source[header.end() : record_end]
        if vnum == 9999999 and record == "$~\n":
            continue
        remainder = record
        for _ in range(4):
            remainder = remainder.split("~\n", 1)[1]
        act_flags = int(remainder.split(None, 1)[0])
        assert not act_flags & 1, f"mob {vnum} persists derived ACT_SPEC"


# --- boot-time item events must have a live worker ---------------------------
# Corpse restoration runs inside boot_db() and records an audit event. Starting
# the item worker afterward forced a flat-file fallback on every clean boot.
comm = (ROOT / "src/comm.c").read_text()
run_game = comm.split("void run_the_game(int port, int sslport)\n{", 1)[1]
run_game = run_game.split("\nstatic int drain_new_connections", 1)[0]
assert index(run_game, "persistence_replay_fallback_events();") < index(
    run_game, "persistence_start_item_event_worker();"
)
assert index(run_game, "persistence_start_item_event_worker();") < index(
    run_game, "boot_db(mini_mode);"
)


print("boot log hygiene contracts passed")
