#!/usr/bin/env python3
"""Regression test for the severity of the death-recovery custody poll (#174).

A player death hands the corpse its items one transaction at a time, so a
well-equipped death spends one retry poll per carried item while the chain
drains. Every one of those polls called persistence_alert(AVATAR, ...), which
broadcasts to every immortal online: a death that was draining correctly, and
finished with extract_refused=0, read on the channel as a database stall.

The wait is expected bounded work -- the branch's own comment said so -- so the
routine poll now goes to the log with outcome=info. The channel
hears about it only when the wait passes DEATH_RECOVERY_STALL_SECONDS, and then
once per stall window rather than once per poll, so a genuine stall is still
visible without being a flood.

The wait is measured against a MONOTONIC CLOCK, not against the number of retry
callbacks that happened to run. ne_events() executes an entry whose tick is due
or late, and a main loop held up by blocking work is exactly the condition the
alert exists to expose; counting callbacks would report one second of waiting
after a thirty-second stall and stay silent.

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
import re
import subprocess
import tempfile

from _paths import SRC
from contract_text import contains, index

fight = (SRC / "fight.c").read_text(encoding="utf-8", errors="replace")
utility = (SRC / "utility.c").read_text(encoding="utf-8", errors="replace")


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


def call_arguments(text, offset):
    """The argument list of the call whose name starts at `offset`, split at
    top-level commas."""
    opened = text.index("(", offset)
    depth = 0
    quoted = False
    escaped = False
    arguments = [""]
    for index_ in range(opened, len(text)):
        character = text[index_]
        if escaped:
            escaped = False
            arguments[-1] += character
            continue
        if character == "\\":
            escaped = True
            arguments[-1] += character
            continue
        if character == '"':
            quoted = not quoted
            arguments[-1] += character
            continue
        if quoted:
            arguments[-1] += character
            continue
        if character in "([":
            depth += 1
            if depth == 1:
                continue
        elif character in ")]":
            depth -= 1
            if depth == 0:
                return [argument.strip() for argument in arguments]
        elif character == "," and depth == 1:
            arguments.append("")
            continue
        arguments[-1] += character
    raise AssertionError("unterminated call at offset %d" % offset)


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

assert contains(busy, "persistence_report(persistence_severity::info,"), (
    "the routine custody wait must be logged, not broadcast to every immortal")
assert "persistence_severity::info" in busy, (
    "the logged wait must be marked expected so it is not read as a failure")
assert contains(busy, "schedule_death_extract_retry(ch, context.corpse_uid, "
                      "DEATH_EXTRACT_RETRY_INITIAL)"), (
    "the poll must keep polling at the steady interval")

# ------------------------------------------------------------------ #
# The wait is a CLOCK, not a callback count                            #
# ------------------------------------------------------------------ #
assert contains(busy, "persistence_observability_now_usec()"), (
    "the wait must be measured against the monotonic clock, because a late or "
    "deferred callback is exactly the stall this alert exists to report")
assert contains(busy, "ch->only.pc->death_custody_wait_since_usec"), (
    "the wait's start must live on the player, so the fallback pulse -- which "
    "rebuilds the event context from scratch -- sees the same clock")
assert contains(busy, "const uint64_t waited_usec = now > since ? now - since : 0"),  (
    "the elapsed wait must be derived from the clock and cannot go negative")
assert contains(busy, "death_custody_wait_should_alert(waited_usec"), (
    "the decision must be taken on elapsed microseconds")
polls_at = index(busy, "const int polls =")
assert "waited_sec" in busy and polls_at > index(busy, "waited_usec"), (
    "polls is diagnostic metadata, derived after the measurement, never the "
    "measurement itself")

# Every path that leaves the wait must clear its clock, or the next handoff
# inherits an elapsed time it never spent and alerts immediately.
assert contains(retry, "death_custody_wait_reset(ch);"), (
    "falling out of the wait must reset its clock")
assert index(retry, "death_custody_wait_reset(ch);") > busy_end, (
    "the reset belongs after the wait, not inside it")
die = body(fight, "void die(P_char ch, P_char killer)\n{")
assert contains(die, "death_custody_wait_reset(ch);"), (
    "a new death must not inherit the previous wait's clock")

# ------------------------------------------------------------------ #
# A wait long enough to be suspicious still reaches the channel        #
# ------------------------------------------------------------------ #
assert contains(busy, "persistence_alert(AVATAR"), (
    "a stalled custody handoff must still alert")
assert contains(busy, "ch->only.pc->death_custody_wait_alerts++"), (
    "each alert must be counted, or the stall re-alerts on every poll")
alert = index(busy, "persistence_alert(AVATAR")
debug = index(busy, "persistence_report(persistence_severity::info,")
assert index(busy, "death_custody_wait_should_alert") < alert < debug, (
    "the alert must be the gated branch and the log the fallback, not the reverse")

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
# Real failures stay loud                                              #
# ------------------------------------------------------------------ #
for action in ("death_recovery_abandoned",
               "death_recovery_schedule_failed",
               "death_disposition_retry",
               "death_recovery_corpse_missing",
               "death_recovery_retry",
               "terminal_save_failed"):
    where = fight.index(action)
    opened = fight.rfind("persistence_alert(", 0, where)
    assert opened >= 0 and "AVATAR" in fight[opened:where], (
        action + " must remain an AVATAR alert")

print("[PASS] the wait is clocked and logged; stalls and failures still alert")

# ------------------------------------------------------------------ #
# The escalation arithmetic, RUN -- and run as the production          #
# function, compiled from the production source with the production    #
# constant, so neither the formula nor the threshold can drift away    #
# from what this test claims about them.                               #
# ------------------------------------------------------------------ #
threshold = re.search(r"#define DEATH_RECOVERY_STALL_SECONDS\s+(\d+)", fight)
assert threshold, "the stall window must be a named constant"
STALL_SECONDS = int(threshold.group(1))
assert STALL_SECONDS == 30, (
    "the documented stall window is 30s; change this pin deliberately, not by "
    "accident -- the PR and the operators' expectations both name it")

decision = body(fight, "static bool death_custody_wait_should_alert(uint64_t waited_usec, "
                       "int alerts)")

HARNESS = """
#include <cassert>
#include <cstdint>
#include <cstdio>

#define DEATH_RECOVERY_STALL_SECONDS %d

%s

int main()
{
        const uint64_t second = 1000000;
        const uint64_t window = (uint64_t)DEATH_RECOVERY_STALL_SECONDS * second;

        // The reported flood: a drain that finishes inside the window is silent
        // however many polls it took, because polls are not the measure.
        for (uint64_t elapsed = 0; elapsed < window; elapsed += second)
                assert(!death_custody_wait_should_alert(elapsed, 0));

        // The first alert lands exactly on the window, not before it.
        assert(death_custody_wait_should_alert(window, 0));
        assert(!death_custody_wait_should_alert(window - 1, 0));

        // Having alerted once, the next is a whole window later: a stall stays
        // visible without becoming the flood this branch was.
        assert(!death_custody_wait_should_alert(window + second, 1));
        assert(!death_custody_wait_should_alert(2 * window - 1, 1));
        assert(death_custody_wait_should_alert(2 * window, 1));

        // A stalled main loop that skipped every intervening poll still reports
        // on the first callback that runs -- the whole point of a clock.
        assert(death_custody_wait_should_alert(10 * window, 0));

        // Five minutes of stall, sampled once a second, alerts once per window.
        int alerts = 0;
        for (uint64_t elapsed = 0; elapsed <= 300 * second; elapsed += second)
                if (death_custody_wait_should_alert(elapsed, alerts))
                        ++alerts;
        assert(alerts == 300 / DEATH_RECOVERY_STALL_SECONDS);

        printf("silent below %%ds; %%d alerts over five minutes\\n",
               DEATH_RECOVERY_STALL_SECONDS, alerts);
        return 0;
}
""" % (STALL_SECONDS, decision)

with tempfile.TemporaryDirectory(prefix="death-alert-level-") as directory:
    source = Path(directory) / "escalation.cpp"
    binary = Path(directory) / "escalation"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Werror", str(source), "-o", str(binary)],
                   check=True)
    subprocess.run([str(binary)], check=True)

print("[PASS] one alert per elapsed window, however the polls happen to fall")

# ------------------------------------------------------------------ #
# persistence_alert renders NO detail unless EVERY conversion is       #
# numeric, and the compiler cannot help: the function carries no       #
# printf format attribute. Run the real validator over every call's    #
# real format rather than looking for one bad conversion by hand.      #
# ------------------------------------------------------------------ #
validator = body(utility, "static int persistence_alert_format_is_numeric(const char *format)")

formats = []
for match in re.finditer(r"persistence_(alert|report)\(", fight):
    arguments = call_arguments(fight, match.start())
    if match[1] == "report":
        arguments = arguments[1:]
    if len(arguments) < 7:
        continue
    detail = arguments[6]
    pieces = re.findall(r'"((?:[^"\\]|\\.)*)"', detail)
    assert pieces, (
        "a persistence_alert detail that is not a literal cannot be checked "
        "here: " + " ".join(detail.split())[:100])
    formats.append("".join(pieces))

assert len(formats) >= 15, "expected every persistence_alert in fight.c, found %d" % len(formats)

CASES = "\n".join(
    '        assert(persistence_alert_format_is_numeric("%s") == 1);' % text
    for text in formats)

VALIDATOR_HARNESS = """
#include <cassert>
#include <cstdio>
#include <cstring>

%s

int main()
{
%s

        // The guard is only worth anything if it rejects what production
        // rejects: these are the conversions that silently blank a detail.
        assert(persistence_alert_format_is_numeric("subsystem=%%s") == 0);
        assert(persistence_alert_format_is_numeric("delay=%%p") == 0);
        assert(persistence_alert_format_is_numeric("who=%%c") == 0);
        assert(persistence_alert_format_is_numeric("count=%%n") == 0);

        printf("%d persistence_alert formats accepted by the real validator\\n");
        return 0;
}
""" % (validator, CASES, len(formats))

with tempfile.TemporaryDirectory(prefix="death-alert-format-") as directory:
    source = Path(directory) / "formats.cpp"
    binary = Path(directory) / "formats"
    source.write_text(VALIDATOR_HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Werror", str(source), "-o", str(binary)],
                   check=True)
    subprocess.run([str(binary)], check=True)

print("[PASS] every persistence_alert detail survives the real format validator")
