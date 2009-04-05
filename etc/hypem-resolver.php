<?php

require_once dirname(__FILE__) . '/phpresolver/playdarresolver.php';

class HypeMachineResolver extends PlaydarResolver
{
    protected $name = 'HypeMachine Resolver';
    protected $targetTime = 150; // Rough guess for a remote service
    protected $weight = 70; 
    
    public function resolve($request)
    {
        $source = sprintf('http://hypem.com/search/"%s"+"%s"/', urlencode($request->artist), urlencode($request->track));
        
        // We need to grab the AUTH cookie we get issued here.
        $html = file_get_contents($source);
        
        preg_match_all('/\({.*?}\)/s', $html, $matches);
        
        $results = array();
        
        foreach($matches[0] as $match) {
            // We might want to filter out some of the fields, they're mostly useless.
            $json = $this->reformatHypeJSON(substr($match, 2, -2));
            $result = json_decode($json);
            
            $result->score = $this->calculateRelevancy($request, $result);
            $result->url = sprintf("http://hypem.com/serve/play/%s/%s", $result->id, $result->key);
            $result->track = $result->song;
            // Use the unique id as a fraction to vary keys where the score is the same.
            $key =  round($result->score) . "-{$result->id}";
            
            $results[$key] = $result;
        }
        
        krsort($results, SORT_NUMERIC);
        return $results;
    }
    
    private function reformatHypeJSON($hypeJSON)
    {
        // Mung and fix json, convert to obj
        
        $pairs = explode("\n", trim($hypeJSON));
        $json = '';
        
        foreach($pairs as $pair) {
            list($key, $val) = explode(":", trim($pair));
            
            $key = trim($key);
            $val = trim($val);
            
            $key = "\"$key\":";
            $val = substr($val, 1, (strrpos($val, ',') === strlen($val)-1 ? -2 : -1));

            $json .=  $key . "\"$val\"" . ',';
        }

        $json = substr($json, 0, -1);
        return '{' . $json . '}';
    }
    
    private function calculateRelevancy($request, $result)
    {
        similar_text($request->artist, $result->artist, $artistSimilarity);
        similar_text($request->track, $result->song, $trackSimilarity);
        
        // Artist or track has nothing in common? Fuck this, i'm out.
        if (!($artistSimilarity && $trackSimilarity)) {
            return 0;
        }

        // Out of 100
        return ($artistSimilarity + $trackSimilarity) / 2;
    }
    
    function proxy($url)
    {
        // get cookie val
        // make request
        // passthru not download
    }
}

$resolver = new HypeMachineResolver();
$resolver->sendReply($resolver->getSettings());
$resolver->handleRequest(fopen("php://STDIN",'r'));