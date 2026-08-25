"""Contract for bounded event catch-up and elapsed-time regen."""

from pathlib import Path

from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text()
events = (ROOT / "src" / "events.c").read_text()

for marker in (
    "NEVENT_CATCHUP_WINDOW_PULSES",
    "nevent_catchup_debt",
    "nevent_catchup_extension_us",
    "NEVENT CATCHUP",
    "avg_callback_us",
):
    assert contains(source, marker), marker

for marker in (
    "struct regen_event_state",
    "last_tick",
    "elapsed_ticks",
):
    assert contains(events, marker), marker


def repayment_quotas(debt: int, window: int = 4) -> list[int]:
    quotas = []
    for remaining in range(window, 0, -1):
        quota = (debt + remaining - 1) // remaining
        quotas.append(quota)
        debt -= quota
    return quotas


def coalesced_regen(
    per_tick: int, elapsed_ticks: int, pulses_in_tick: int, accumulated: float = 0.0
) -> tuple[int, float]:
    accumulated += (per_tick * elapsed_ticks) / pulses_in_tick
    whole = int(accumulated)
    return whole, accumulated - whole


assert repayment_quotas(300) == [75, 75, 75, 75]
assert repayment_quotas(301) == [76, 75, 75, 75]
assert coalesced_regen(100, 4, 100) == (4, 0.0)
assert coalesced_regen(25, 4, 100) == (1, 0.0)

print("nevent dynamic catch-up and regen coalescing contract passed")
