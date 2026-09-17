-- Additive migration 0021: identity-stable Collector notification outbox.
--
-- Existing offline_messages rows are legacy rows and retain a NULL message_id.
-- New Collector notifications are keyed by (pid, message_id) and retain a
-- receipt after the physical queue row is acknowledged/deleted.

SET @offline_message_id_sql = IF(
    EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = DATABASE()
          AND table_name = 'offline_messages'
          AND column_name = 'message_id'
    ),
    'SELECT 1',
    'ALTER TABLE offline_messages ADD COLUMN message_id BINARY(16) NULL DEFAULT NULL AFTER pid'
);
PREPARE offline_message_id_stmt FROM @offline_message_id_sql;
EXECUTE offline_message_id_stmt;
DEALLOCATE PREPARE offline_message_id_stmt;

CREATE TABLE IF NOT EXISTS offline_message_receipts (
    pid INT NOT NULL,
    message_id BINARY(16) NOT NULL,
    message MEDIUMTEXT NOT NULL,
    status TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attempt_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    last_attempt_at TIMESTAMP(6) NULL DEFAULT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    delivered_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (pid, message_id),
    KEY idx_offline_message_receipt_pending
        (pid, status, last_attempt_at, created_at, message_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

SET @offline_message_identity_index_sql = IF(
    (
        SELECT COUNT(*)
        FROM information_schema.statistics
        WHERE table_schema = DATABASE()
          AND table_name = 'offline_messages'
          AND index_name = 'uq_offline_message_identity'
    ) = 2
    AND (
        SELECT COUNT(*)
        FROM information_schema.statistics
        WHERE table_schema = DATABASE()
          AND table_name = 'offline_messages'
          AND index_name = 'uq_offline_message_identity'
          AND non_unique = 0
          AND sub_part IS NULL
          AND ((seq_in_index = 1 AND column_name = 'pid')
               OR (seq_in_index = 2 AND column_name = 'message_id'))
    ) = 2,
    'SELECT 1',
    'ALTER TABLE offline_messages ADD UNIQUE KEY uq_offline_message_identity (pid, message_id)'
);
PREPARE offline_message_identity_index_stmt FROM @offline_message_identity_index_sql;
EXECUTE offline_message_identity_index_stmt;
DEALLOCATE PREPARE offline_message_identity_index_stmt;

SET @offline_receipt_pending_index_sql = IF(
    (
        SELECT COUNT(*)
        FROM information_schema.statistics
        WHERE table_schema = DATABASE()
          AND table_name = 'offline_message_receipts'
          AND index_name = 'idx_offline_message_receipt_pending'
    ) = 5
    AND (
        SELECT COUNT(*)
        FROM information_schema.statistics
        WHERE table_schema = DATABASE()
          AND table_name = 'offline_message_receipts'
          AND index_name = 'idx_offline_message_receipt_pending'
          AND non_unique = 1
          AND sub_part IS NULL
          AND (
              (seq_in_index = 1 AND column_name = 'pid')
              OR (seq_in_index = 2 AND column_name = 'status')
              OR (seq_in_index = 3 AND column_name = 'last_attempt_at')
              OR (seq_in_index = 4 AND column_name = 'created_at')
              OR (seq_in_index = 5 AND column_name = 'message_id')
          )
    ) = 5,
    'SELECT 1',
    'ALTER TABLE offline_message_receipts ADD KEY idx_offline_message_receipt_pending (pid, status, last_attempt_at, created_at, message_id)'
);
PREPARE offline_receipt_pending_index_stmt FROM @offline_receipt_pending_index_sql;
EXECUTE offline_receipt_pending_index_stmt;
DEALLOCATE PREPARE offline_receipt_pending_index_stmt;
