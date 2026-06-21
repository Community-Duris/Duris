-- dirty saves support: add unique constraints for upsert pattern
-- run this before enabling redis dirty saves

-- player_languages: unique on (pid, tongue_id)
ALTER TABLE player_languages
  ADD UNIQUE KEY uk_pid_tongue (pid, tongue_id);

-- player_intros: unique on (pid, intro_index)
ALTER TABLE player_intros
  ADD UNIQUE KEY uk_pid_intro (pid, intro_index);

-- player_timers: unique on (pid, timer_id)
ALTER TABLE player_timers
  ADD UNIQUE KEY uk_pid_timer (pid, timer_id);

-- player_undead_slots: unique on (pid, circle)
ALTER TABLE player_undead_slots
  ADD UNIQUE KEY uk_pid_circle (pid, circle);

-- player_forged_items: unique on (pid, forge_index)
ALTER TABLE player_forged_items
  ADD UNIQUE KEY uk_pid_forge (pid, forge_index);

-- player_granted_cmds: unique on (pid, cmd_num)
ALTER TABLE player_granted_cmds
  ADD UNIQUE KEY uk_pid_cmd (pid, cmd_num);

-- player_skills: unique on (pid, skill_id)
ALTER TABLE player_skills
  ADD UNIQUE KEY uk_pid_skill (pid, skill_id);
