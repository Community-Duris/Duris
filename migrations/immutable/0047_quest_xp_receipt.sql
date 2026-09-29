-- Fence committed quest XP effects with the corresponding player snapshot.
SET @quest_xp_mask_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'quest_reward_obligation'
      AND column_name = 'xp_applied_mask'
);
SET @quest_xp_mask_ddl = IF(
    @quest_xp_mask_exists = 0,
    'ALTER TABLE quest_reward_obligation '
    'ADD COLUMN xp_applied_mask BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER continuation',
    'SELECT 1'
);
PREPARE quest_xp_mask_stmt FROM @quest_xp_mask_ddl;
EXECUTE quest_xp_mask_stmt;
DEALLOCATE PREPARE quest_xp_mask_stmt;
