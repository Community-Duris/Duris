#!/usr/bin/env python3
"""Regression test for the severity of the death-recovery custody poll (#174).

A player death hands the corpse its items one transaction at a time, so a
well-equipped death spends one retry poll per carried item while the chain
drains. Every one of those polls called persistence_alert(AVATAR, ...), which
broadcasts to every immortal online: a death that was draining correctly, and
finished with extract_refused=0, read on the channel as a database stall.

The wait is expected bounded work -- the branch's own comment said so -- so the
routine poll now goes to the log at LOG_DEBUG with outcome=expected. The channel
hears about it only when the wait passes DEATH_RECOVERY_STALL_SECONDS, and then
once per stall window rather than once per poll, so a genuine stall is still
visible without being a flood.

Two reporting bugs went with it. The message was named for corpse items while
its condition is equally true of a wallet conversion still in flight, sending
readers to the wrong subsystem; and it printed a delay in PULSES that reads like
seconds, which is how a 13-second drain was reported as 52 seconds. Both
subsystems are now named and the wait is reported in seconds.

Every branch that is an actual failure -- an abandoned recovery, a refused event
admission, a disputed handoff, a missing corpse, a failed terminal save -- stays
at AVATAR. This test fails if any of them is demoted along with the poll.
"""

from pathlib import Path
import subprocess
import sys
import tempfile

from _paths import SRC
from contract_text import contains, index

fight = (SRC / "fight.c").read_text(encoding="utf-8", errors="replace")


def body(text, signature):
    """One function, from its signature through its closing brace."""
    start = index(text, signature)
    depth = 0
    for offset in range(text.index("{", start), len(text)):
        if text[offset] == "{":
            depth += 1
        elif text[offset] == "}":
            depth -= 1
            if depth == 0:
                return text[start:offset + 1]
    raise AssertionError("unterminated function: " + signature)


# The prototype is not the definition: ask for the signature WITH its opening
# brace so the forward declaration a few lines above cannot answer instead.
retry = body(fight, "static void event_death_extract_retry(P_char ch, P_char victim, "
                    "P_obj obj, void *data)\n{")

# ------------------------------------------------------------------ #
# The routine poll is a log line, not a broadcast                      #
# ------------------------------------------------------------------ #
busy_start = index(retry, "if (items_busy || currency_busy)")
busy_end = retry.index("return;", busy_start)
busy = retry[busy_start:busy_end]

assert contains(busy, "logit(LOG_DEBUG,"), (
    "the routine custody wait must be logged, not broadcast to every immortal")
assert "outcome=expected" in busy, (
    "the logged wait must be marked expected so it is not read as a failure")
assert contains(busy, "schedule_death_extract_retry(ch, context.corpse_uid, "
                      "DEATH_EXTRACT_RETRY_INITIAL, polls)"), (
    "the poll must keep polling at the steady interval and carry its count")

# ------------------------------------------------------------------ #
# A wait long enough to be suspicious still reaches the channel        #
# ------------------------------------------------------------------ #
assert "DEATH_RECOVERY_STALL_SECONDS" in busy, (
    "escalation must be gated on the stall window, not on every poll")
assert contains(busy, "persistence_alert(AVATAR"), (
    "a stalled custody handoff must still alert")
escalation = index(busy, "DEATH_RECOVERY_STALL_SECONDS")
alert = index(busy, "persistence_alert(AVATAR")
debug = index(busy, "logit(LOG_DEBUG,")
assert escalation < alert < debug, (
    "the alert must be the gated branch and the log the fallback, not the reverse")

# The gate itself, not merely a mention of the window. The native harness below
# re-implements this expression to prove what it does; pinning the original is
# what keeps that copy honest, because a lifted copy cannot notice the source
# drifting away from it.
assert contains(busy, "if (waited_sec / DEATH_RECOVERY_STALL_SECONDS > "
                      "before_sec / DEATH_RECOVERY_STALL_SECONDS)"), (
    "the alert must fire only on the poll that crosses into a new stall window")

# ------------------------------------------------------------------ #
# The message says which subsystem is busy, and how long in seconds    #
# ------------------------------------------------------------------ #
for field in ("items=%d", "currency=%d", "polls=%d", "waited_sec=%d"):
    assert field in busy, "the custody wait must report " + field
assert "death_recovery_awaiting_custody" in busy, (
    "the action must not claim corpse items for a wait a wallet conversion "
    "can equally cause")
assert "death_recovery_awaiting_corpse_items" not in fight, (
    "the misleading action name must be gone, not merely joined by a new one")

# ------------------------------------------------------------------ #
# persistence_alert renders NO detail unless every conversion is       #
# numeric (persistence_alert_format_is_numeric in utility.c), so a %s  #
# in a detail silently prints nothing at all.                          #
# ------------------------------------------------------------------ #
offset = 0
while True:
    offset = fight.find("persistence_alert(", offset)
    if offset < 0:
        break
    depth = 0
    for end in range(fight.index("(", offset), len(fight)):
        if fight[end] == "(":
            depth += 1
        elif fight[end] == ")":
            depth -= 1
            if depth == 0:
                break
    call = fight[offset:end + 1]
    assert "%s" not in call, (
        "persistence_alert drops a detail containing %s: " + " ".join(call.split())[:120])
    offset = end

# ------------------------------------------------------------------ #
# Real failures stay loud                                              #
# ------------------------------------------------------------------ #
for action in ("death_recovery_abandoned",
               "death_recovery_schedule_failed",
               "death_disposition_retry",
               "death_recovery_corpse_missing",
               "death_recovery_retry",
               "death_recovery_restarting_corpse_items",
               "corpse_items_in_flight",
               "corpse_items_restart"):
    where = fight.index(action)
    opened = fight.rfind("persistence_alert(", 0, where)
    assert opened >= 0 and "AVATAR" in fight[opened:where], (
        action + " must remain an AVATAR alert")

print("[PASS] the routine custody poll is logged; stalls and failures still alert")

# ------------------------------------------------------------------ #
# The escalation arithmetic, run rather than read: one alert per stall #
# window, and none at all for a drain that finishes inside one.        #
# ------------------------------------------------------------------ #
HARNESS = r"""
#include <cassert>
#include <cstdio>

#define WAIT_SEC 4
#define DEATH_EXTRACT_RETRY_INITIAL 4
#define DEATH_RECOVERY_STALL_SECONDS 30

// Lifted from event_death_extract_retry's busy branch.
static bool alerts_on_poll(int previous_polls)
{
        const int polls = previous_polls + 1;
        const int waited_sec = polls * DEATH_EXTRACT_RETRY_INITIAL / WAIT_SEC;
        const int before_sec = previous_polls * DEATH_EXTRACT_RETRY_INITIAL / WAIT_SEC;
        return waited_sec / DEATH_RECOVERY_STALL_SECONDS >
               before_sec / DEATH_RECOVERY_STALL_SECONDS;
}

int main()
{
        // The reported flood: 13 polls for a 13-item corpse. Not one alert.
        for (int poll = 0; poll < 13; ++poll)
                assert(!alerts_on_poll(poll));

        // A stall does not go unheard, and is not a flood either.
        int alerts = 0;
        for (int poll = 0; poll < 300; ++poll)
                if (alerts_on_poll(poll))
                        ++alerts;
        const int seconds = 300 * DEATH_EXTRACT_RETRY_INITIAL / WAIT_SEC;
        assert(alerts == seconds / DEATH_RECOVERY_STALL_SECONDS);
        assert(alerts == 10);

        printf("%d polls: 0 alerts under the window, %d over five minutes\n", 13, alerts);
        return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="death-alert-level-") as directory:
    source = Path(directory) / "escalation.cpp"
    binary = Path(directory) / "escalation"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Werror", str(source), "-o", str(binary)],
                   check=True)
    subprocess.run([str(binary)], check=True)

print("[PASS] one alert per stall window, none for a drain that finishes inside one")
