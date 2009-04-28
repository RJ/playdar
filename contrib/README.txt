This dir contains experimental and non-essential resolvers. 

The easiest way to build a plugin atm is to move or symlink the dir from
contrib/ into resolvers/ then run "make" as usual, eg:

$ cd resolvers/
$ ln -s ../contrib/demo-resolver .
$ cd build/
$ make demo

This should configure and build demo. 
Once built, you'll have a "demo.resolver" in the plugins/ dir.
All .resolver files in plugins/ are auto-loaded at startup.

Resolvers are either C++ plugins or scripts in perl/python/etc.
See http://www.playdar.org/resolvers.html for details.
