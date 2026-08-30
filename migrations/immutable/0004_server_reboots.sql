-- Immutable migration 0004: add the durable server lifecycle records written
-- by scripts/cycle_mud.sh after each completed runtime invocation. Launchers
-- before b029 created this table directly with an id column, signed INT epoch
-- fields, a VARCHAR shutdown type, created_at, and four legacy indexes. When
-- that shape is present, copy every row into the canonical table and atomically
-- swap it into place. A failed attempt leaves either the original authoritative
-- table or the complete canonical replacement at server_reboots, so retrying is
-- safe.

CREATE TABLE IF NOT EXISTS server_reboots (
    record_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    boot_time BIGINT UNSIGNED NOT NULL,
    shutdown_time BIGINT UNSIGNED NOT NULL,
    uptime_seconds BIGINT UNSIGNED NOT NULL,
    shutdown_type ENUM(
        'shutdown',
        'reboot',
        'copyover',
        'autoreboot',
        'pwipe',
        'hung',
        'autoreboot_copyover',
        'crash',
        'unknown'
    ) NOT NULL DEFAULT 'unknown',
    initiated_by VARCHAR(255) NULL DEFAULT NULL,
    reason TEXT NULL DEFAULT NULL,
    PRIMARY KEY (record_id),
    KEY idx_server_reboots_boot_time (boot_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELIMITER //
CREATE PROCEDURE migrate_0004_server_reboots()
BEGIN
    DECLARE legacy_shape INT DEFAULT 0;

    SELECT COUNT(*) INTO legacy_shape
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'server_reboots'
      AND column_name = 'id';

    IF legacy_shape = 1 THEN
        DROP TABLE IF EXISTS server_reboots_0004;
        CREATE TABLE server_reboots_0004 (
            record_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            boot_time BIGINT UNSIGNED NOT NULL,
            shutdown_time BIGINT UNSIGNED NOT NULL,
            uptime_seconds BIGINT UNSIGNED NOT NULL,
            shutdown_type ENUM(
                'shutdown',
                'reboot',
                'copyover',
                'autoreboot',
                'pwipe',
                'hung',
                'autoreboot_copyover',
                'crash',
                'unknown'
            ) NOT NULL DEFAULT 'unknown',
            initiated_by VARCHAR(255) NULL DEFAULT NULL,
            reason TEXT NULL DEFAULT NULL,
            PRIMARY KEY (record_id),
            KEY idx_server_reboots_boot_time (boot_time)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

        INSERT INTO server_reboots_0004 (
            record_id,
            boot_time,
            shutdown_time,
            uptime_seconds,
            shutdown_type,
            initiated_by,
            reason
        )
        SELECT
            id,
            boot_time,
            shutdown_time,
            uptime_seconds,
            CASE
                WHEN shutdown_type IN (
                    'shutdown',
                    'reboot',
                    'copyover',
                    'autoreboot',
                    'pwipe',
                    'hung',
                    'autoreboot_copyover',
                    'crash',
                    'unknown'
                ) THEN shutdown_type
                ELSE 'unknown'
            END,
            initiated_by,
            reason
        FROM server_reboots
        ORDER BY id;

        DROP TABLE IF EXISTS server_reboots_0004_legacy;
        RENAME TABLE
            server_reboots TO server_reboots_0004_legacy,
            server_reboots_0004 TO server_reboots;
        DROP TABLE server_reboots_0004_legacy;
    ELSE
        -- Clean up only migration-owned scratch names after an interrupted
        -- post-swap attempt. The canonical server_reboots table is authoritative.
        DROP TABLE IF EXISTS server_reboots_0004;
        DROP TABLE IF EXISTS server_reboots_0004_legacy;
    END IF;
END//
CALL migrate_0004_server_reboots()//
DROP PROCEDURE migrate_0004_server_reboots//
DELIMITER ;
