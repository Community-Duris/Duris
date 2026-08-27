-- Every public locker item receives the same durable public-chest context used by
-- private chests. Temporary locker rooms and sorting chest objects are never owners.
START TRANSACTION;

INSERT IGNORE INTO private_chests(locker_id,chest_name,is_public)
SELECT id,'public',1 FROM lockers;

UPDATE locker_items item
JOIN private_chests public_chest
  ON public_chest.locker_id=item.locker_id AND public_chest.is_public=1
SET item.chest_id=public_chest.id
WHERE item.chest_id IS NULL;

INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision)
SELECT DISTINCT 5,item.locker_id,item.chest_id,0
FROM item_current_owner current_item
JOIN item_ownership_baseline baseline ON baseline.item_uid=current_item.item_uid
JOIN locker_items item
  ON baseline.source_table='locker_items' AND item.id=baseline.source_row_id
WHERE current_item.owner_type=5 AND current_item.item_revision=0;

UPDATE item_current_owner current_item
JOIN item_ownership_baseline baseline ON baseline.item_uid=current_item.item_uid
JOIN locker_items item
  ON baseline.source_table='locker_items' AND item.id=baseline.source_row_id
SET current_item.owner_id=item.locker_id,
    current_item.owner_context_id=item.chest_id
WHERE current_item.owner_type=5 AND current_item.item_revision=0;

UPDATE item_ownership_baseline baseline
JOIN locker_items item
  ON baseline.source_table='locker_items' AND item.id=baseline.source_row_id
SET baseline.owner_id=item.locker_id,
    baseline.owner_context_id=item.chest_id
WHERE baseline.owner_type=5 AND baseline.opening_item_revision=0;

COMMIT;
