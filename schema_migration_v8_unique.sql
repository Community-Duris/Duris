-- schema migration v8: add unique constraints for character names

-- first, clean up any existing duplicates (keep lowest pid)
DELETE ac1 FROM account_characters ac1
INNER JOIN account_characters ac2
WHERE ac1.char_name = ac2.char_name
  AND ac1.pid > ac2.pid;

-- add unique constraint on char_name (character names must be unique)
ALTER TABLE account_characters
  ADD UNIQUE INDEX idx_char_name_unique (char_name);

-- add unique constraint on player_data.name
ALTER TABLE player_data
  ADD UNIQUE INDEX idx_player_name_unique (name);
