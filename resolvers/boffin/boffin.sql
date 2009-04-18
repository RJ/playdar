-- Community tags

CREATE TABLE IF NOT EXISTS tag (
	name TEXT NOT NULL
);

CREATE UNIQUE INDEX tag_name_idx ON tag(name);


CREATE TABLE IF NOT EXISTS file_tag (
	file INTEGER NOT NULL,
	tag INTEGER NOT NULL,
	weight FLOAT NOT NULL
);

CREATE INDEX file_tag_track_idx ON file_tag(file);
CREATE INDEX file_tag_tag_idx ON file_tag(tag);
