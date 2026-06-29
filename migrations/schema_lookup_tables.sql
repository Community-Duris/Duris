-- lookup tables for races and classes
-- these get populated by the mud on every boot

DROP TABLE IF EXISTS classes;
DROP TABLE IF EXISTS races;

CREATE TABLE races (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    short_name VARCHAR(32),
    ansi_name VARCHAR(128),
    abbrev VARCHAR(4),
    racewar TINYINT DEFAULT 0 COMMENT '0=neutral, 1=good, 2=evil',
    playable TINYINT DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE classes (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    ansi_name VARCHAR(128),
    short_name VARCHAR(8),
    menu_char CHAR(1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- optional: create a view to replace players_core
-- this joins player_data with lookup tables for human-readable names
CREATE OR REPLACE VIEW players_view AS
SELECT
    pd.pid,
    pd.name,
    pd.level,
    pd.race as race_id,
    r.ansi_name as race,
    pd.m_class as class_id,
    c.ansi_name as classname,
    pd.racewar,
    pd.assoc_id,
    pd.exp,
    pd.epics,
    pd.played_time as playtime,
    (pd.copper + pd.silver*10 + pd.gold*100 + pd.platinum*1000) as money,
    (pd.bank_copper + pd.bank_silver*10 + pd.bank_gold*100 + pd.bank_platinum*1000) as balance
FROM player_data pd
LEFT JOIN races r ON pd.race = r.id
LEFT JOIN classes c ON pd.m_class = c.id;
