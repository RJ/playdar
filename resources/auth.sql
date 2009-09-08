-- HTTP Authentication 

DROP TABLE IF EXISTS playdar_auth;
CREATE TABLE IF NOT EXISTS playdar_auth (
    token TEXT NOT NULL PRIMARY KEY,
    website TEXT NOT NULL,
    name TEXT NOT NULL,
    ua TEXT NOT NULL,
    mtime INTEGER NOT NULL,
    permissions TEXT NOT NULL
);

-- Schema version and misc settings

CREATE TABLE IF NOT EXISTS settings (
    key TEXT NOT NULL PRIMARY KEY,
    value TEXT NOT NULL DEFAULT ''
);
INSERT INTO settings(key ,value) VALUES('schema_version', '2');
