-- dirty saves support: add unique constraints for upsert pattern
-- run this before enabling redis dirty saves
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- player_languages: unique on (pid, tongue_id)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_languages'
    AND index_name = 'uk_pid_tongue');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_languages ADD UNIQUE KEY uk_pid_tongue (pid, tongue_id)',
    'SELECT "uk_pid_tongue already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_intros: unique on (pid, intro_index)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_intros'
    AND index_name = 'uk_pid_intro');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_intros ADD UNIQUE KEY uk_pid_intro (pid, intro_index)',
    'SELECT "uk_pid_intro already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_timers: unique on (pid, timer_id)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_timers'
    AND index_name = 'uk_pid_timer');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_timers ADD UNIQUE KEY uk_pid_timer (pid, timer_id)',
    'SELECT "uk_pid_timer already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_undead_slots: unique on (pid, circle)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_undead_slots'
    AND index_name = 'uk_pid_circle');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_undead_slots ADD UNIQUE KEY uk_pid_circle (pid, circle)',
    'SELECT "uk_pid_circle already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_forged_items: unique on (pid, forge_index)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_forged_items'
    AND index_name = 'uk_pid_forge');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_forged_items ADD UNIQUE KEY uk_pid_forge (pid, forge_index)',
    'SELECT "uk_pid_forge already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_granted_cmds: unique on (pid, cmd_num)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_granted_cmds'
    AND index_name = 'uk_pid_cmd');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_granted_cmds ADD UNIQUE KEY uk_pid_cmd (pid, cmd_num)',
    'SELECT "uk_pid_cmd already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_skills: unique on (pid, skill_id)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_skills'
    AND index_name = 'uk_pid_skill');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_skills ADD UNIQUE KEY uk_pid_skill (pid, skill_id)',
    'SELECT "uk_pid_skill already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
