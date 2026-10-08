# Journal loading for existing capitalized area filenames

Preparing The Royal Mausoleum of Castle IceCrag exposed an implementation defect: its registered source filename is `Voluntown`, but Python story validation allowed only lowercase area names. The C++ journal loader used the same lowercase identifier rule and skipped the capitalized registry entry before attempting to read its file. An otherwise valid `areas/story/Voluntown.story.json` could not load.

This separate implementation fix accepts ASCII uppercase letters only for the registered `source_area` and its matching filename. It preserves exact case, registry ownership, native completion bindings and future story identifiers. It does not lowercase or rename the zone. Story IDs, step IDs, NPC command keywords and topics keep their existing lowercase rules. Slashes, dots, spaces, traversal strings and a differently cased unregistered source name remain invalid.

The focused regression uses the actual `Voluntown` registry entry and both native caretaker contracts. It checks Python projection and native direct/file loading, preservation of the capitalized story identity, rejection of wrong case and traversal, and continued rejection of uppercase story/step IDs. The old Python validator reproduced a failure on the valid registered-case fixture before the fix. The new C++ direct and directory loader paths are exercised after the fix.

This is a journal-loader repair suitable for news: **Zone journals now support existing capitalized area filenames, preserving the zone's registered name and quest history.** No room, exit, key, trap, mobile, reset or native quest data is changed. The journal's content is added in a subsequent feature commit.

Validation passed: the focused Python/native C++ direct and directory-loader regression (seven acceptance/rejection cases) on Linux and Windows, all147 compiled Windows journal journeys, and changed/staged canonical formatting. The maintained Linux server build passed with make -C src using g++-12. No played accounting journey is claimed. Accounting gates remain required for new progress; schema acceptance does not enable accounting or award gameplay credit.
