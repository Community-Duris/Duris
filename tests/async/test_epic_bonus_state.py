#!/usr/bin/env python3
"""Runtime coverage for the bounded in-memory epic bonus state."""

import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "epic_bonus_state.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>

static bool near(double left, double right) { return fabs(left - right) < 0.000001; }

int main(void)
{
	struct EpicBonusState state = {};
	if (!near(epic_bonus_state_modifier(&state, 3, 100), 0.0)) return 1;
	epic_bonus_state_mark_unavailable(&state);
	if (state.status != EPIC_BONUS_STATE_UNAVAILABLE ||
	    !near(epic_bonus_state_modifier(&state, 3, 100), 0.0)) return 2;

	struct EpicBonusContributionBucket buckets[] = {{200, 2500}, {300, 7500}};
	if (!epic_bonus_state_publish(&state, 3, 50, 10000.0, 0.8, buckets, 2, 100)) return 3;
	if (state.status != EPIC_BONUS_STATE_READY || state.total_contributions != 10000 ||
	    state.next_expiry != 200 || !near(epic_bonus_state_modifier(&state, 3, 100), 0.8) ||
	    !near(epic_bonus_state_modifier(&state, 4, 100), 0.0)) return 4;
	if (!near(epic_bonus_state_modifier(&state, 4, 200), 0.0) ||
	    state.total_contributions != 7500 || state.next_expiry != 300) return 5;
	if (!near(epic_bonus_state_modifier(&state, 3, 200), 0.6) ||
	    state.total_contributions != 7500 || state.next_expiry != 300) return 6;
	if (!near(epic_bonus_state_modifier(&state, 3, 300), 0.0) || state.bucket_count != 0 ||
	    state.next_expiry != 0) return 7;

	if (!epic_bonus_state_select(&state, 5, 400, 1000.0, 2.0) ||
	    state.selected_type != 5 || state.selected_at != 400 || state.bucket_count != 0) return 7;
	if (!epic_bonus_state_add(&state, 100, 600, 450) ||
	    !epic_bonus_state_add(&state, 200, 500, 450) ||
	    !epic_bonus_state_add(&state, 300, 500, 450)) return 8;
	if (state.bucket_count != 2 || state.buckets[0].expires_at != 500 ||
	    state.buckets[0].amount != 500 || state.buckets[1].expires_at != 600 ||
	    state.total_contributions != 600 ||
	    !near(epic_bonus_state_modifier(&state, 5, 450), 1.2)) return 9;

	const struct EpicBonusState before_invalid = state;
	struct EpicBonusContributionBucket unsorted[] = {{700, 1}, {650, 1}};
	if (epic_bonus_state_publish(&state, 3, 1, 10.0, 1.0, unsorted, 2, 1) ||
	    state.selected_type != before_invalid.selected_type ||
	    state.total_contributions != before_invalid.total_contributions) return 10;
	if (!near(epic_bonus_state_modifier(&state, 5, 400), 1.2) || state.bucket_count != 2) return 11;

	if (!epic_bonus_state_select(&state, 2, 1, 1.0, 1.0) ||
	    !epic_bonus_state_add(&state, INT64_MAX, 1000, 2) ||
	    !epic_bonus_state_add(&state, 100, 1000, 2) ||
	    state.total_contributions != INT64_MAX || state.buckets[0].amount != INT64_MAX) return 12;

	if (!epic_bonus_state_select(&state, 1, 1, 100.0, 0.4)) return 13;
	for (int i = 0; i < EPIC_BONUS_STATE_MAX_BUCKETS; ++i)
		if (!epic_bonus_state_add(&state, 1, 100 + i, 2)) return 14;
	if (epic_bonus_state_add(&state, 1, 1000, 2) || state.bucket_count != 32) return 15;

	if (epic_bonus_state_publish(&state, -1, 1, 1.0, 1.0, NULL, 0, 1) ||
	    epic_bonus_state_publish(&state, 1, -1, 1.0, 1.0, NULL, 0, 1) ||
	    epic_bonus_state_publish(&state, 1, 1, 0.0, 1.0, NULL, 0, 1) ||
	    epic_bonus_state_publish(&state, 1, 1, 1.0, -1.0, NULL, 0, 1)) return 16;

	puts("epic bonus state runtime checks passed");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-epic-bonus-state-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS)
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{SRC}",
         str(harness), str(SRC / "epic_bonus_state.c"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("epic bonus state tests passed")
