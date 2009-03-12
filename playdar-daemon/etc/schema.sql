-- the basic 3 catalogue tables:
CREATE TABLE IF NOT EXISTS artist (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    sortname TEXT NOT NULL
);
CREATE UNIQUE INDEX artist_sortname ON artist(sortname);

CREATE TABLE IF NOT EXISTS track (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    artist INTEGER NOT NULL REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE,
    name TEXT NOT NULL,
    sortname TEXT NOT NULL
);
CREATE UNIQUE INDEX track_artist_sortname ON track(artist,sortname);

CREATE TABLE IF NOT EXISTS album (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    artist INTEGER NOT NULL REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE,
    name TEXT NOT NULL,
    sortname TEXT NOT NULL
);
CREATE UNIQUE INDEX album_artist_sortname ON album(artist,sortname);

-- the trigram search indexes
CREATE TABLE IF NOT EXISTS artist_search_index (
    ngram TEXT NOT NULL,
    id INTEGER NOT NULL REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE,
    num INTEGER NOT NULL DEFAULT 1
);
CREATE UNIQUE INDEX artist_search_index_ngram_artist ON artist_search_index(ngram, id);

CREATE TABLE IF NOT EXISTS album_search_index (
    ngram TEXT NOT NULL,
    id INTEGER NOT NULL REFERENCES album(id) ON DELETE CASCADE ON UPDATE CASCADE,
    num INTEGER NOT NULL DEFAULT 1
);
CREATE UNIQUE INDEX album_search_index_ngram_album ON album_search_index(ngram, id);

CREATE TABLE IF NOT EXISTS track_search_index (
    ngram TEXT NOT NULL,
    id INTEGER NOT NULL REFERENCES track(id) ON DELETE CASCADE ON UPDATE CASCADE,
    num INTEGER NOT NULL DEFAULT 1
);
CREATE UNIQUE INDEX track_search_index_ngram_track ON track_search_index(ngram, id);




--CREATE TABLE IF NOT EXISTS album_track (
--    album INTEGER NOT NULL REFERENCES album(id) ON DELETE CASCADE ON UPDATE CASCADE,
--    track INTEGER NOT NULL REFERENCES track(id) ON DELETE CASCADE ON UPDATE CASCADE,
--    position INTEGER NOT NULL
--);



-- files on disk and joinage with catalogue:
CREATE TABLE IF NOT EXISTS file (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT NOT NULL,
    size INTEGER NOT NULL,
    mtime INTEGER NOT NULL,
    md5 TEXT,
    mimetype TEXT,
    duration INTEGER NOT NULL DEFAULT 0,
    bitrate INTEGER NOT NULL DEfAULT 0
);
CREATE UNIQUE INDEX file_path_uniq ON file(path);

CREATE TABLE IF NOT EXISTS file_join (
    file INTEGER NOT NULL REFERENCES file(id) ON DELETE CASCADE ON UPDATE CASCADE,
    artist INTEGER NOT NULL REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE,
    track INTEGER NOT NULL REFERENCES track(id) ON DELETE CASCADE ON UPDATE CASCADE,
    album INTEGER REFERENCES album(id) ON DELETE CASCADE ON UPDATE CASCADE
);

-- HTTP Authentication 

CREATE TABLE IF NOT EXISTS playdar_auth (
    token TEXT NOT NULL PRIMARY KEY,
    website TEXT NOT NULL,
    name TEXT NOT NULL,
    mtime INTEGER NOT NULL,
    permissions TEXT NOT NULL
);


