-- Normalize corpse custody to a stable collision-free player-pid/save-id identity.
-- corpses.id is replaced by every legacy save, while save_id alone can collide when
-- two players die in the same second.
START TRANSACTION;

INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision)
SELECT DISTINCT 4,
       ((CAST(pd.pid AS UNSIGNED) << 32) | (CAST(c.save_id AS UNSIGNED) & 4294967295)),0,0
FROM item_current_owner current_item
JOIN item_ownership_baseline baseline ON baseline.item_uid=current_item.item_uid
JOIN corpse_items ci ON baseline.source_table='corpse_items' AND ci.id=baseline.source_row_id
JOIN corpses c ON c.id=ci.corpse_id
JOIN player_data pd ON LOWER(pd.name)=LOWER(c.player_name)
WHERE current_item.owner_type=4 AND current_item.item_revision=0;

UPDATE item_current_owner current_item
JOIN item_ownership_baseline baseline ON baseline.item_uid=current_item.item_uid
JOIN corpse_items ci ON baseline.source_table='corpse_items' AND ci.id=baseline.source_row_id
JOIN corpses c ON c.id=ci.corpse_id
JOIN player_data pd ON LOWER(pd.name)=LOWER(c.player_name)
SET current_item.owner_id=
      ((CAST(pd.pid AS UNSIGNED) << 32) | (CAST(c.save_id AS UNSIGNED) & 4294967295))
WHERE current_item.owner_type=4 AND current_item.item_revision=0;

UPDATE item_ownership_baseline baseline
JOIN corpse_items ci ON baseline.source_table='corpse_items' AND ci.id=baseline.source_row_id
JOIN corpses c ON c.id=ci.corpse_id
JOIN player_data pd ON LOWER(pd.name)=LOWER(c.player_name)
SET baseline.owner_id=
      ((CAST(pd.pid AS UNSIGNED) << 32) | (CAST(c.save_id AS UNSIGNED) & 4294967295))
WHERE baseline.owner_type=4 AND baseline.opening_item_revision=0;

DELETE owner_revision FROM item_owner_revision owner_revision
LEFT JOIN item_current_owner current_item
  ON current_item.owner_type=owner_revision.owner_type
 AND current_item.owner_id=owner_revision.owner_id
 AND current_item.owner_context_id=owner_revision.owner_context_id
WHERE owner_revision.owner_type=4 AND owner_revision.revision=0
  AND current_item.item_uid IS NULL;

COMMIT;
