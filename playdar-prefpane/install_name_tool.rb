#!/usr/bin/ruby

def opt( f )
  lines = `otool -L #{f}`.split( "\n" )
  lines.shift
  lines.each do |line|
    path = line.strip.split[0]
    yield( path, "@executable_path/#{File.basename( path )}" ) if path.match( '^/opt' )
  end
end

Dir['*'].each do |f|
  `install_name_tool -id #{f} #{f}` if File.extname( f ) == '.dylib'
  opt( f ) { |from, to| `install_name_tool -change #{from} #{to} #{f}` }
end