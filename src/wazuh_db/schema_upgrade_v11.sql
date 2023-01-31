/*
 * SQL Schema for upgrading databases
 * Copyright (C) 2015, Wazuh Inc.
 *
 * Febrary 1, 2023.
 *
 * This program is a free software, you can redistribute it
 * and/or modify it under the terms of GPLv2.
*/

BEGIN;

CREATE TABLE IF NOT EXISTS sys_hwinfo_tmp (
    scan_id INTEGER,
    scan_time TEXT,
    board_serial TEXT,
    cpu_name TEXT,
    cpu_cores INTEGER,
    cpu_mhz REAL,
    ram_total INTEGER,
    ram_free INTEGER,
    ram_usage INTEGER,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    PRIMARY KEY (scan_id, board_serial)
);

INSERT INTO sys_hwinfo_tmp SELECT * FROM sys_hwinfo;
DROP TABLE sys_hwinfo;
ALTER TABLE sys_hwinfo_tmp RENAME TO sys_hwinfo;

INSERT OR REPLACE INTO metadata (key, value) VALUES ('db_version', 11);

COMMIT;
