#!/usr/bin/python
# Created by Max Howell <max@methylblue.com> twitter.com/mxcl
# Licensed the same as Playdar
#
# Uses Python 2.5 and our own bundled simplejson

######################################################################## imports
import hmac, hashlib, time
import urllib, urllib2
from xml.dom import minidom
from struct import unpack, pack
import simplejson as json
import sys

###################################################################### functions
def print_json(o):
    s = json.dumps(o)
    sys.stdout.write(pack('!L', len(s)))
    sys.stdout.write(s)
    sys.stdout.flush()

def seeqpod_request(artist, track):
    timestamp = str(time.time())
    path = '/api/v0.2/music/search/'+urllib.quote_plus(artist + ' ' + track)
    sig = hmac.new('adf88bc774f77081079543d6370f51c6e564b035', digestmod=hashlib.sha1)
    sig.update(path)
    sig.update(timestamp)
    
    r = urllib2.Request('http://www.seeqpod.com'+path)
    r.add_header('Seeqpod-uid', '8d7ae66644a71ec03102490a7e737b2e42610610')
    r.add_header('Seeqpod-call-signature', sig.hexdigest())
    r.add_header('Seeqpod-timestamp', timestamp)
    r.add_header('User-Agent', 'Playdar')
    return r

def percent_encode(url):
    # Ha! Yeah, needs work
    return url.replace(' ', '%20')

def element_value(e, name, throw = False):
    try:
        return e.getElementsByTagName(name)[0].firstChild.nodeValue
    except:
        if throw:
            raise
        return ""

def resolve(artist, track):
    request = seeqpod_request(artist, track)
    response = urllib2.urlopen(request)
    tracks = []
    for e in minidom.parseString(response.read()).getElementsByTagName('track'):
        try:
            t = dict()
            t["artist"] = element_value(e, 'creator')
            t["track"]  = element_value(e, 'title')
            t["album"]  = element_value(e, 'album')
            t["url"]    = percent_encode(element_value(e, 'location'), True)
            t["source"] = 'SeeqPod'
            tracks.append(t)
        except:
            pass
    return tracks

####################################################################### settings
settings = dict()
settings["settings"] = True
settings["name"] = "SeeqPod Resolver"
settings["targettime"] = 1000 # millseconds
settings["weight"] = 50 # seeqpod results aren't as good as friend's results
print_json( settings )

###################################################################### main loop
while 1:
    length = sys.stdin.read(4)
    length = unpack('!L', length)[0]
    if not length:
        break
    # something probably went wrong, most likely we're out of sync and are 
    # reading the 4 bytes length header in the middle of a json string. We can't
    # recover. Bail.
    if length > 4096 or length < 0:
        break
    if length > 0:
        msg = sys.stdin.read(length)
        try:
            request = json.loads(msg)
            tracks = resolve(request['artist'], request['track'])
            if len(tracks) > 0:
                response = { 'qid':request['qid'], 'results':tracks }
                print_json(response)
        except:
            # safe to continue, skipping this msg, because at least
            # we consumed enough input so next iteration hits size header.
            pass

