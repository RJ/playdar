#!/bin/sh
# Created by Max Howell on 28/02/2009.

# prep
d=`dirname "$2"`
mkdir -p "$d"
rm "$2"
sqlite3 "$2" < schema.sql

# go
tmp=`mktemp -t playdar`
echo "'"`pwd`"/../MacOS/scanner' '$2' '$1'; rm '$tmp'" >> "$tmp"
chmod u+x "$tmp"
open -a Terminal "$tmp"
