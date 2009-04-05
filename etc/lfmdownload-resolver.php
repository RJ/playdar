<?php

require_once dirname(__FILE__) . '/phpresolver/playdarresolver.php';

class LastfmDownloadResolver extends PlaydarResolver
{
    protected $name = 'Last.fm Free Download';
    protected $targetTime = 15; // fast atm, it's all hardcoded.
    protected $weight = 80; // 1-100. higher means preferable.
    
    public function resolve($request)
    {
        $source = sprintf("http://last.fm/music/%s/_/%s", urlencode($request->artist), urlencode($request->track));
        $html = file_get_contents($source);
        
        preg_match('/http:\/\/freedownloads.last.fm\/.*?.mp3/s', $html, $matches);
        
        if ($matches) {
            $result = array(
                'artist' => $request->artist,
                'track' => $request->track,
                'score' => 100,
                'url' => $matches[0],
                'bitrate' => 128,
            );
            return array((Object) $result);
        }
        return array();
    }
}

$resolver = new LastfmDownloadResolver();
$resolver->sendReply($resolver->getSettings());
$resolver->handleRequest(fopen("php://STDIN",'r'));