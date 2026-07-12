from pathlib import Path

root = Path(__file__).resolve().parents[2]
utility = (root / "src/utility.c").read_text()
prototypes = (root / "src/prototypes.h").read_text()
actwiz = (root / "src/actwiz.c").read_text()

assert "int persistence_quarantine_fallback_events(void)" in utility
assert "access(LOG_EVENT, F_OK)" in utility
assert '"%s.pwipe-quarantine.%ld.%ld"' in utility
assert "rename(LOG_EVENT, quarantine_path)" in utility
assert "persistence_quarantine_fallback_events(void);" in prototypes
prepare = actwiz.index("if (!persistence_prepare_pwipe())")
quarantine = actwiz.index("if (!persistence_quarantine_fallback_events())", prepare)
wipe = actwiz.index("if (!sql_pwipe(1723699))", quarantine)
assert prepare < quarantine < wipe
print("pwipe fallback quarantine checks passed")
