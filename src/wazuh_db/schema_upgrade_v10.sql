/*
 * SQL Schema for upgrading databases
 * Copyright (C) 2015, Wazuh Inc.
 *
 * Dec 22, 2022
 *
 * This program is a free software, you can redistribute it
 * and/or modify it under the terms of GPLv2.
*/

UPDATE sca_check SET result='not applicable' WHERE status='Not applicable';
ALTER TABLE sca_check DROP COLUMN status;

CREATE TABLE IF NOT EXISTS _sys_programs (
    scan_id INTEGER,
    scan_time TEXT,
    format TEXT NOT NULL CHECK (format IN ('pacman', 'deb', 'rpm', 'win', 'pkg', 'apk')),
    name TEXT,
    priority TEXT,
    section TEXT,
    size INTEGER CHECK (size >= 0),
    vendor TEXT,
    install_time TEXT,
    version TEXT,
    architecture TEXT,
    multiarch TEXT,
    source TEXT,
    description TEXT,
    location TEXT,
    triaged INTEGER(1),
    cpe TEXT,
    msu_name TEXT,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    item_id TEXT,
    PRIMARY KEY (scan_id, name, version, architecture)
);

INSERT INTO _sys_programs SELECT scan_id, scan_time, format, name, priority, section, size, vendor, install_time, version, architecture, multiarch, source, description, location, triaged, cpe, msu_name, CASE WHEN checksum <> '' THEN checksum ELSE 'legacy' END AS checksum, item_id FROM sys_programs;
DROP TABLE IF EXISTS sys_programs;
ALTER TABLE _sys_programs RENAME TO sys_programs;
CREATE INDEX IF NOT EXISTS programs_id ON sys_programs (scan_id);

INSERT OR REPLACE INTO metadata (key, value) VALUES ('db_version', 10);
