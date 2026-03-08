-- Player table schema
-- Based on PlayerInfo message from player.proto

CREATE TABLE IF NOT EXISTS player (
    player_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    name VARCHAR(64) NOT NULL,
    level INT NOT NULL DEFAULT 1,
    exp BIGINT NOT NULL DEFAULT 0,
    hp INT NOT NULL DEFAULT 100,
    max_hp INT NOT NULL DEFAULT 100,
    mp INT NOT NULL DEFAULT 100,
    max_mp INT NOT NULL DEFAULT 100,
    pos_x FLOAT NOT NULL DEFAULT 0.0,
    pos_y FLOAT NOT NULL DEFAULT 0.0,
    pos_z FLOAT NOT NULL DEFAULT 0.0,
    rot_x FLOAT NOT NULL DEFAULT 0.0,
    rot_y FLOAT NOT NULL DEFAULT 0.0,
    rot_z FLOAT NOT NULL DEFAULT 0.0,
    profession INT NOT NULL DEFAULT 0,
    gender INT NOT NULL DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (player_id),
    UNIQUE KEY idx_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Create index for faster lookups
CREATE INDEX idx_player_level ON player(level);
CREATE INDEX idx_player_profession ON player(profession);
