# Database Schema

This directory contains the database schema for the MMO server.

## Tables

### player

The `player` table stores player information based on the `PlayerInfo` message from `player.proto`.

#### Schema

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| player_id | BIGINT UNSIGNED | AUTO_INCREMENT | Unique player ID |
| name | VARCHAR(64) | NOT NULL | Player name |
| level | INT | 1 | Player level |
| exp | BIGINT | 0 | Player experience |
| hp | INT | 100 | Current HP |
| max_hp | INT | 100 | Maximum HP |
| mp | INT | 100 | Current MP |
| max_mp | INT | 100 | Maximum MP |
| pos_x | FLOAT | 0.0 | Position X coordinate |
| pos_y | FLOAT | 0.0 | Position Y coordinate |
| pos_z | FLOAT | 0.0 | Position Z coordinate |
| rot_x | FLOAT | 0.0 | Rotation X coordinate |
| rot_y | FLOAT | 0.0 | Rotation Y coordinate |
| rot_z | FLOAT | 0.0 | Rotation Z coordinate |
| profession | INT | 0 | Player profession/class |
| gender | INT | 0 | Player gender |
| created_at | TIMESTAMP | CURRENT_TIMESTAMP | Record creation time |
| updated_at | TIMESTAMP | CURRENT_TIMESTAMP | Record update time |

#### Indexes

- PRIMARY KEY: `player_id`
- UNIQUE KEY: `name`
- INDEX: `level`
- INDEX: `profession`

## Usage

To create the database and tables:

```sql
-- Create database
CREATE DATABASE IF NOT EXISTS mmo_server CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- Use database
USE mmo_server;

-- Create tables
SOURCE player.sql;
```

Or run the SQL file directly:

```bash
mysql -u root -p mmo_server < database/player.sql
```
