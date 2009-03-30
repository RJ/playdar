#!/usr/bin/python
#!/opt/local/bin/python2.6

import hmac, sha, time

import urllib
import urllib2
from xml.dom import minidom
from struct import unpack, pack
import json
import sys

# settings
settings_string  = '{'
settings_string += '"settings":true,'
settings_string += '"name":"Seeqpod Resolver (python 2.6)",'
settings_string += '"targettime":500,'
settings_string += '"weight":40' # we're relatively shite
settings_string += '}'

def print_json(s):
    jsono = json.loads(s)
    jsons = json.dumps(jsono)
    sys.stdout.write(pack('!L', len(jsons)))
    sys.stdout.write(jsons)
    sys.stdout.flush()

print_json(settings_string)


def request(artist, track):
    timestamp = str(time.time())
    path = '/api/v0.2/music/search/'+urllib.quote_plus(artist + ' ' + track)
    myhmac = hmac.new('adf88bc774f77081079543d6370f51c6e564b035', digestmod=sha)
    for i in (path, timestamp):
        myhmac.update(i)
    sig = myhmac.hexdigest()

    request = urllib2.Request('http://www.seeqpod.com'+path)
    request.add_header('Seeqpod-uid', '8d7ae66644a71ec03102490a7e737b2e42610610')
    request.add_header('Seeqpod-call-signature', sig)
    request.add_header('Seeqpod-timestamp', timestamp)
    request.add_header('User-Agent', 'Playdar')
    return request

def resolve(qid, artist, track):
    r = request(artist, track)

    response = urllib2.urlopen(r)
    xml = minidom.parseString(response.read())
    tracks = xml.getElementsByTagName('track')

    for t in tracks:
        artist = t.getElementsByTagName('creator')[0].firstChild.nodeValue
        track = t.getElementsByTagName('title')[0].firstChild.nodeValue
        url = t.getElementsByTagName('location')[0].firstChild.nodeValue
        print_json( '{ "qid":"%s", "results": [ { "artist":"%s", "track":"%s", "album":"Album", "source":"SeeqPod", "url":"%s", "size":3000000, "bitrate":128, "duration":240, "score":0.50} ] }' % (qid, artist, track, url) )
        return

while 1:
    length = sys.stdin.read(4)
    if not length:
        sys.exit()
    length = unpack('!L', length)[0]
    
    if length > 0:
        jsons = sys.stdin.read(length)
        jsono = json.loads(jsons)
        resolve(jsono['qid'], jsono['artist'], jsono['track'])
