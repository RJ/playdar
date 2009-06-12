Please get the library it from here:
http://curl.haxx.se/download/libcurl-7.19.3-win32-ssl-msvc.zip
(release)

to link, you will need to put curllib.lib in debug/lib and release/lib

IMPORTANT: It MUST be the DLL version! Static does not seem to work.

Therefore DO NOT FORGET to put:
  curllib.dll
  libeay32.dll
  openldap.dll
  ssleay32.dll
in the same directory as the .exe files
