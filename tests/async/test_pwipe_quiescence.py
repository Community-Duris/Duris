from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
prototypes = (ROOT / "src/prototypes.h").read_text()
utility = (ROOT / "src/utility.c").read_text()
actwiz = (ROOT / "src/actwiz.c").read_text()
comm = (ROOT / "src/comm.c").read_text()

assert "persistence_prepare_pwipe" in prototypes
assert "persistence_prepare_pwipe" in utility
pwipe_start = actwiz.index("case TimedShutdownData::PWIPE:")
pwipe_case = actwiz[pwipe_start:actwiz.index("default:", pwipe_start)]
assert "shutdownflag = _pwipe = 1" in pwipe_case
assert pwipe_case.index("shutdownflag = _pwipe = 1") < pwipe_case.index("persistence_prepare_pwipe")
assert "shutdownData.eShutdownType = TimedShutdownData::NONE" in pwipe_case
assert pwipe_case.count("shutdownflag = _pwipe = 0") >= 3
assert "persistence_large_event_queue_reset();" in utility
assert "persistence_scalar_event_queue_reset();" in utility
assert "persistence_item_event_queue_reset();" in utility
assert utility.index("persistence_large_event_worker_stop(0)") < utility.index("persistence_large_event_queue_reset()")
assert utility.index("persistence_large_event_queue_reset()") < utility.index("PWipe persistence workers quiesced")

assert "if (!_copyover && !_pwipe)\n\t{\n\t\tpersistence_flush_all_character_saves();" in comm
assert "if (!_pwipe)" in comm[comm.index("game_loop(port, sslport);"):comm.index("/* Don't need this anymore")]

print("pwipe quiescence checks passed")
