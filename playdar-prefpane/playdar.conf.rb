#!/usr/bin/ruby
# Created by Max Howell on 01/03/2009.

contents = File.new( 'playdar.conf' ).read
contents.sub!( 'YOURNAMEHERE', ARGV[0] )
contents.sub!( 'collection.db', ARGV[1] )
File.open( ARGV[2], 'w' ) { |f| f.write( contents ) }

puts "Created #{ARGV[2]}"
