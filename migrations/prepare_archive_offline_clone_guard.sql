-- Prepare only a newly restored, isolated disposable archive clone. The
-- legacy runner refuses archive-only column removal without this marker.
-- Bind it to the current server and schema so a restored copy does not inherit
-- a usable attestation on a different target. This does not quiesce live writers.
CREATE TABLE duris_archive_offline_clone_guard (
    guard_id TINYINT UNSIGNED NOT NULL PRIMARY KEY,
    server_name VARCHAR(255) NOT NULL,
    database_name VARCHAR(128) NOT NULL
) ENGINE=InnoDB;
INSERT INTO duris_archive_offline_clone_guard (guard_id,server_name,database_name)
SELECT 1, @@hostname, DATABASE();
