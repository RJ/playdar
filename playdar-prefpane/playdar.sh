#!/bin/sh
# Created by Max Howell on 05/03/2009.

tmp=`mktemp -t playdar`
echo "'"`pwd`"/../MacOS/playdar' $@; rm $tmp" >> "$tmp"
chmod u+x "$tmp"
open -a Terminal "$tmp"
