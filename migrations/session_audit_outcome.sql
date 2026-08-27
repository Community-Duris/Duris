-- Phase 02 typed, privacy-minimized login/logout audit outcome.
CREATE TABLE IF NOT EXISTS session_audit_outcome (
    operation_id BINARY(16) NOT NULL,
    pid INT UNSIGNED NOT NULL,
    event_type TINYINT UNSIGNED NOT NULL,
    occurred_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    KEY idx_session_audit_player_time (pid, occurred_at),
    KEY idx_session_audit_event_time (event_type, occurred_at),
    CONSTRAINT session_audit_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_session_audit_event CHECK (event_type IN (1,2))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
