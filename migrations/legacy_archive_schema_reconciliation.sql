-- Reconcile columns found on pre-canonical archived production schemas.
-- Full source rows are copied outside the runtime contract BEFORE any column
-- removal. The sidecars are durable import evidence, not disposable scratch.
-- Reject pre-existing sidecars and partial ship schemas rather than guessing
-- whether a previous attempt or another writer changed their contents.
DROP PROCEDURE IF EXISTS duris_archive_legacy_ship_columns;
DROP PROCEDURE IF EXISTS duris_archive_legacy_locker_item_type;
DELIMITER //
CREATE PROCEDURE duris_archive_legacy_ship_columns()
BEGIN
    DECLARE legacy_columns INT DEFAULT 0;
    DECLARE source_rows BIGINT DEFAULT 0;
    DECLARE copied_rows BIGINT DEFAULT 0;

    SELECT COUNT(*) INTO legacy_columns
    FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='ships'
      AND column_name IN ('owner_pid','crew_index','crew_sail_skill',
          'crew_guns_skill','crew_rpar_skill','crew_sail_chief',
          'crew_guns_chief','crew_rpar_chief','maxspeed_bonus',
          'capacity_bonus');
    IF legacy_columns > 0 THEN
        IF NOT EXISTS (SELECT 1 FROM information_schema.tables
                       WHERE table_schema=DATABASE()
                         AND table_name='duris_archive_offline_clone_guard') THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='archive columns require an isolated clone marker';
        END IF;
        IF (SELECT COUNT(*) FROM duris_archive_offline_clone_guard
            WHERE guard_id=1 AND server_name=@@hostname
              AND database_name=DATABASE()) <> 1 THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='archive clone marker does not match target';
        END IF;
        IF EXISTS (SELECT 1 FROM information_schema.tables
                   WHERE table_schema=DATABASE()
                     AND table_name='legacy_import_archive_ships') THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='ship archive sidecar already exists';
        END IF;
        IF legacy_columns <> 10 THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='partial legacy ship columns; no changes made';
        END IF;
        IF EXISTS (
            SELECT 1 FROM information_schema.key_column_usage k
            WHERE (k.table_schema=DATABASE() AND k.table_name='ships'
                   AND k.referenced_table_name IS NOT NULL
                   AND k.column_name IN ('owner_pid','crew_index','crew_sail_skill',
                       'crew_guns_skill','crew_rpar_skill','crew_sail_chief',
                       'crew_guns_chief','crew_rpar_chief','maxspeed_bonus',
                       'capacity_bonus'))
               OR (k.referenced_table_schema=DATABASE()
                   AND k.referenced_table_name='ships'
                   AND k.referenced_column_name IN ('owner_pid','crew_index',
                       'crew_sail_skill','crew_guns_skill','crew_rpar_skill',
                       'crew_sail_chief','crew_guns_chief','crew_rpar_chief',
                       'maxspeed_bonus','capacity_bonus'))
        ) THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='FK references legacy ship columns; no changes made';
        END IF;
        SELECT COUNT(*) INTO source_rows FROM ships;
        CREATE TABLE legacy_import_archive_ships LIKE ships;
        INSERT INTO legacy_import_archive_ships SELECT * FROM ships;
        SELECT COUNT(*) INTO copied_rows FROM legacy_import_archive_ships;
        IF source_rows <> copied_rows THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='ship archive row copy incomplete';
        END IF;
        IF EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema=DATABASE() AND table_name='ships'
                     AND index_name='idx_ships_owner_pid') THEN
            ALTER TABLE ships DROP INDEX idx_ships_owner_pid;
        END IF;
        ALTER TABLE ships
            DROP COLUMN owner_pid,
            DROP COLUMN crew_index,
            DROP COLUMN crew_sail_skill,
            DROP COLUMN crew_guns_skill,
            DROP COLUMN crew_rpar_skill,
            DROP COLUMN crew_sail_chief,
            DROP COLUMN crew_guns_chief,
            DROP COLUMN crew_rpar_chief,
            DROP COLUMN maxspeed_bonus,
            DROP COLUMN capacity_bonus;
    END IF;
END //
CREATE PROCEDURE duris_archive_legacy_locker_item_type()
BEGIN
    DECLARE source_rows BIGINT DEFAULT 0;
    DECLARE copied_rows BIGINT DEFAULT 0;
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema=DATABASE()
                 AND table_name='account_locker_items' AND column_name='item_type') THEN
        IF NOT EXISTS (SELECT 1 FROM information_schema.tables
                       WHERE table_schema=DATABASE()
                         AND table_name='duris_archive_offline_clone_guard') THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='archive columns require an isolated clone marker';
        END IF;
        IF (SELECT COUNT(*) FROM duris_archive_offline_clone_guard
            WHERE guard_id=1 AND server_name=@@hostname
              AND database_name=DATABASE()) <> 1 THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='archive clone marker does not match target';
        END IF;
        IF EXISTS (SELECT 1 FROM information_schema.tables
                   WHERE table_schema=DATABASE()
                     AND table_name='legacy_import_archive_account_locker_items') THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='locker item archive sidecar already exists';
        END IF;
        IF EXISTS (
            SELECT 1 FROM information_schema.key_column_usage k
            WHERE (k.table_schema=DATABASE() AND k.table_name='account_locker_items'
                   AND k.column_name='item_type' AND k.referenced_table_name IS NOT NULL)
               OR (k.referenced_table_schema=DATABASE()
                   AND k.referenced_table_name='account_locker_items'
                   AND k.referenced_column_name='item_type')
        ) THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='FK references legacy locker item_type; no changes made';
        END IF;
        SELECT COUNT(*) INTO source_rows FROM account_locker_items;
        CREATE TABLE legacy_import_archive_account_locker_items LIKE account_locker_items;
        INSERT INTO legacy_import_archive_account_locker_items
            SELECT * FROM account_locker_items;
        SELECT COUNT(*) INTO copied_rows
            FROM legacy_import_archive_account_locker_items;
        IF source_rows <> copied_rows THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='locker item archive row copy incomplete';
        END IF;
        ALTER TABLE account_locker_items DROP COLUMN item_type;
    END IF;
END //
DELIMITER ;
CALL duris_archive_legacy_ship_columns();
CALL duris_archive_legacy_locker_item_type();
DROP PROCEDURE duris_archive_legacy_ship_columns;
DROP PROCEDURE duris_archive_legacy_locker_item_type;
