#!/usr/bin/ruby
# Created by Max Howell on 01/03/2009.

contents = File.new( 'playdar.ini' ).read
contents.sub!( 'name="YOURNAMEHERE"', "name=\"#{ARGV[0]}\"" )
contents.sub!( 'db="collection.db"', "db=\"#{ARGV[1]}\"" )
File.open( ARGV[2], 'w' ) { |f| f.write( contents ) }

puts "Created #{ARGV[2]}"
