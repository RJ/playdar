#!/bin/sh
#@author Max Howell <max@last.fm>

if [[ "$1" != "--db-only" ]]
then
    # clearly, we have to be started with Audioscrobbler.bundle/Contents/MacOS
    # as the working directory, then we explicitly delete the directory by name
    # this is a safety precaution
    cd ../../../
    rm -rf Audioscrobbler.bundle
fi

cd ~/Library/Application\ Support/Last.fm || exit 1

# automatic iPod database
rm iTunesPlays.db
# manual iPod databases
find devices -name playcounts.db -delete
