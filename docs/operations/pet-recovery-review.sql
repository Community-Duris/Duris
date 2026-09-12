-- Read-only report. Apply migration 0013 before using this report.
-- These are candidates for investigation, never proof that an item is disposable.
-- Do not clear a hold or rewrite restore_state without reviewing ownership and
-- arranging an explicit disposition of every equipment UID.
SELECT p.owner_pid, p.id AS pet_id, p.mob_vnum, p.pet_order,
       p.hold_reason, LENGTH(p.restore_state) AS encoded_state_bytes,
       COUNT(i.id) AS saved_item_rows
FROM player_pets AS p
LEFT JOIN player_pet_items AS i ON i.pet_id = p.id
WHERE p.hold_reason <> 0
   OR ((p.restore_state IS NULL OR p.restore_state = '')
       AND (p.mob_vnum = 1201 OR p.mob_vnum BETWEEN 3 AND 10
            OR p.mob_vnum BETWEEN 78 AND 85
            OR p.mob_vnum BETWEEN 33 AND 35 OR p.mob_vnum = 77))
GROUP BY p.owner_pid, p.id, p.mob_vnum, p.pet_order, p.hold_reason,
         LENGTH(p.restore_state)
ORDER BY p.owner_pid, p.pet_order;
