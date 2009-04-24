<?php

/**
 * @author David Singleton (http://dsingleton.co.uk)
 * A base player resolver written in PHP.
 * Handles basic request/response, encoding.
 */

abstract class PlaydarResolver
{
    protected $name; 
    protected $targetTime; // Lower is better
    protected $weight; // 1-100. higher means preferable.
    
    
    public function __construct()
    {
        set_error_handler(array($this, 'errorHandler'), E_ALL);
    }
    
    /**
     * 
     */
    public function handleRequest($fh)
    {
        while (!feof($fh)) {

            // Makes the handler compatable with command line testing and playdar resolver pipeline usage
            if (!$content = fread($fh, 4)) {
                break;
            }
            
            // get the length of the payload from the first 4 bytes:
            $len = current(unpack('N', $content));
            
            // bail on empty request.
            if($len == 0) {
                continue;
            }
            
            // read $len bytes for the actual payload and assume it's a JSON object.
            $request = json_decode(fread($fh, $len));
            
            // Malformed request
            if (!isset($request->artist, $request->album)) {
                continue;
            }
            
            // Let's resolve this bitch
            $results = $this->resolve($request);
            
            // No results, bail
            if (!$results) {
                continue;
            }
            
            // Build response and send
            $response = (Object) array(
                '_msgtype' => 'results',
                'qid' => $request->qid,
                'results' => $results,
            );
            $this->sendResponse($response);
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
    public function sendResponse($response)
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
        $settings = (Object) array(
            '_msgtype' => 'settings',
            'name' => $this->name,
            'targettime' => $this->targetTime,
            'weight' => $this->weight,
        );
        return $settings;
    }
    
    public function log($message)
    {
        $fh = fopen("php://STDERR", 'w');
        fwrite($fh, $message . "\n");
        fclose($fh);
    }
    
    public function errorHandler($errno, $errstr, $errfile, $errline)
    {
        $exit = false;
        
        switch ($errno) {
            case E_USER_ERROR:
                $type = "Fatal";
                $exit = true;

            case E_WARNING:
            case E_USER_WARNING:
                $type = "Warning";
                break;

            case E_NOTICE:
            case E_USER_NOTICE:
                $type = "Notice";
                break;

            default:
                $type = "Unknown";
                break;
        }
        
        $format = 'PHP ' . $type . ' Error: "%s" (line %s in %s)';
        $error = sprintf($format, $errstr, $errline, $errfile);
        
        $this->log($error);
        
        if ($exit) {
            exit(1);
        }
        else {
            /* Don't execute PHP internal error handler */
            return true;
        }
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