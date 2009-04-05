<?php

/**
 * A base player resolver written in PHP.
 * Handles basic request/response, encoding.
 */

abstract class PlaydarResolver
{
    protected $name; 
    protected $targetTime; // Lower is better
    protected $weight; // 1-100. higher means preferable.
    
    protected $resultFields = array('artist', 'track', 'source', 'size', 'bitrate', 'duration', 'url', 'score');
    
    /**
     * 
     */
    public function handleRequest($in)
    {
        while(!feof($in)){
            // get the length of the payload from the first 4 bytes:
            $len = 0;
            $lenA = unpack('N', fread($in, 4));
            $len = $lenA[1];
            if($len == 0) continue;
            // read $len bytes for the actual payload and assume it's a JSON object.
            $msg = fread($in, $len);
            $rq = json_decode($msg);
            // TODO validation - was it valid json?
            $pis = $this->resolve($rq);
            // don't reply with anything if there were no matches:
            if(count($pis)==0) continue;
            $res = new stdclass;
            $res->_msgtype = "results";
            $res->qid = $rq->qid;
            $res->results = array();
            $res->results = $pis;

            $this->sendReply($res);
            
            // After finding results this continues reading and chokes on the input. Could 
            // be a flaw in my test script that generates the playdar requst, fuck knows, bail.
            break; 
        }
    }
    
    /**
     * Find shit. Returns an array of result object
     */
    abstract function resolve($request);
    
    /**
     * Output reply
     * Puts a 4-byte big-endian int first, denoting length of message
     */
    public function sendReply($response)
    {
        // i think json_spirit rejects \/ even tho it's valid json. doh.
        $str = str_replace('\/','/',json_encode($response));
        print pack('N', strlen($str));
        print $str;
    }
    
    /**
     * Settings object for this resolver, reported when we start
     */
    public function getSettings()
    {
        $settings = array(
            '_msgtype' => 'settings',
            'name' => $this->name,
            'targettime' => $this->targetTime,
            'weight' => $this->weight,
        );
        return (Object) $settings;
    }
    
    function getEmptyResult()
    {
        $emptyResult = array();
        foreach($this->resultFields as $field) {
            $emptyResult[$field] = null;
        }
        
        
        return $emptyResult;
    }
}

/**
 * Example implementation of base PlaydarResolver
 */
class ExamplePHPResolver extends PlaydarResolver
{
    protected $name = 'php resolver script';
    protected $targetTime = 15; // fast atm, it's all hardcoded.
    protected $weight = 80; // 1-100. higher means preferable.
    
    public function resolve($request)
    {
        if(strtolower(trim($request->artist))!='mokele') return array();
        if(soundex($request->track)!=soundex('hiding in your insides')) return array();
        $pi = new stdclass;
        $pi->artist = "Mokele";
        $pi->track = "Hiding In Your Insides";
        $pi->album = "You Yourself are Me Myself and I am in Love";
        $pi->source = "Mokele.co.uk";
        $pi->size = 4971780;
        $pi->bitrate= 160;
        $pi->duration = 248;
        // NB this url should be url encoded properly:
        $pi->url = "http://play.mokele.co.uk/music/Hiding%20In%20Your%20Insides.mp3";
        $pi->score = (float)1.00;
        return array($pi);
    }
}