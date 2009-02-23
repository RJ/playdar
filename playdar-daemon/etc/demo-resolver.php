#!/usr/bin/php
<?php
/*
    This is an example, experimental resolving script.
    If you enable the gateway-http resolver, it will spawn this script and feed
    it JSON queries to stdin. This script is expected to reply on stdout with
    JSON if it finds a match.

    This means you can write resolvers in any language, even PHP.
    
    This script is only capable of matching 1 song at the moment:
        Artist: Big Bad Sun
        Track:  Sweet Melissa
    Try searching for it using the search demo at playdar.org 
    with the resolver enabled.

    Current Caveats: the HTTPStreamingStrategy doesnt do name resolving, and
    *requires* a port number, so only (eg) http://1.2.3.4:80/foo.mp3 is valid.

*/

$in = fopen("php://STDIN",'r');
while($line = fgets($in)){
    $line = trim($line);
    if(!$line) continue;
    $rq=null;
    if(null==($rq = json_decode($line))){
        continue;
    }
    $res = new stdclass;
    $res->qid = $rq->qid;
    $res->results = array();
    $pis = get_matches($rq);
    if(count($pis)==0) continue;
    $res->results = $pis;
    // json_spirit rejects \/ even tho it's valid json. doh.
    print str_replace('\/','/',json_encode($res));
    print "\n";
}


function get_matches($rq){
    // this script only matches one song, to a magnatune url:
    if(strtolower(trim($rq->artist))!='big bad sun')  return array();
    if(soundex($rq->track)!=soundex('sweet melissa')) return array();
    $pi = new stdclass;
    $pi->artist = "Big Bad Sun";
    $pi->track  = "Sweet Melissa";
    $pi->source = "Magnatune";
    $pi->bitrate= 128;
    $pi->url    = "http://64.62.148.2:80/all/01-Sweet%20Melissa-Big%20Bad%20Sun.mp3";
    $pi->score  = (float)1i.0;
    return array($pi);
}
