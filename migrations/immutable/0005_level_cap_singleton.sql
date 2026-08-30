-- Immutable migration 0005: repair fresh-bootstrap databases whose level_cap
-- table was created without its required singleton row. Existing live state is
-- never overwritten; an unexpected non-singleton shape is rejected by the
-- verifier for explicit operator repair.

INSERT INTO level_cap (
    id,
    most_frags,
    racewar_leader,
    level,
    next_update
)
SELECT 1, 0, 2, 56, NOW()
FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM level_cap);
