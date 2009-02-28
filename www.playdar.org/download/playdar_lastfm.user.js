// ==UserScript==
// @name          Playdar_Last.fm
// @namespace     http://www.playdar.org/
// @description	  Add Playdar links to Last.fm sites
// @include       http://www.last.fm/*
// @include       http://www.lastfm.jp/*                                                    
// @include       http://www.lastfm.de/*                                                    
// @include       http://www.lastfm.pt/*                                                    
// @include       http://www.lastfm.pl/*                                                    
// @include       http://www.lastfm.co.kr/*                                                 
// @include       http://www.lastfm.es/*                                                    
// @include       http://www.lastfm.fr/*                                                    
// @include       http://www.lastfm.it/*
// @include       http://www.lastfm.ru/*
// @include       http://www.lastfm.no/*
// @include       http://www.lastfm.com.tr/*
// @include       http://www.lastfm.fi/*
// @include       http://www.lastfm.se/*
// @include       http://www.lastfm.nl/*
// @include       http://www.lastfm.dk/*
// @include       http://www.lastfm.at/*
// @include       http://www.lastfm.ch/*
// @include       http://www.lastfm.com.br/*
// ==/UserScript==

// Load the playdar.js
var e = document.createElement('script');
// same js is served, this is jsut for log-grepping ease.
e.src = 'http://www.playdar.org/static/playdar.js?greasemonkey';
document.getElementsByTagName("head")[0].appendChild(e);

function GM_wait() {
    if (typeof unsafeWindow.Playdar == 'undefined') {
        window.setTimeout(GM_wait,100); 
    } else { 
        setup_playdar();
    }
}
GM_wait(); // wait for playdar.js to load.

function setup_playdar() {
    Playdar = unsafeWindow.Playdar;
    var playdar = Playdar.create({
        detected: function (v) {
            //alert('detected, v' + v);
            insert_play_buttons(playdar);
        },
        not_detected: function () {
            //alert('Playdar not detected, script disabled');
        }
    });
    playdar.init();
}

function insert_play_buttons (playdar) {
    // set up a handler for resolved content
    function results_handler (response, finalanswer) {
        if (finalanswer) {
            var element = unsafeWindow.document.getElementById(response.qid);
            if (response.results.length){
                // generate tooltip with details:
                var tt = "Sources: ";
                for (var i = 0; i < response.results.length; i++) {
                    var result = response.results[i];
                    tt += result.source + "/" + result.bitrate + "kbps/" + Playdar.mmss(result.duration) + " ";
                }
                // update status element:
                element.setAttribute('style', 'border:0; margin:0; background-color: lightgreen;  width:12px; height:10px; font-size:9px;margin-right:5px;');
                //var swfstr = "<object height=\"10\" width=\"10\"><embed src=\"http://www.playdar.org/static/player.swf?&song_url=" + playdar.get_stream_url(response.results[0].sid) + "\" height=\"10\" width=\"10\"></embed></object>";
                // Just link to the source, too much flash spam:
                element.innerHTML = "&nbsp;<a href=\"" + playdar.get_stream_url(response.results[0].sid) + "\" title=\"" + tt + "\">" + response.results.length + "</a>&nbsp;";
            } else {
                element.setAttribute('style', 'border:0; margin:0; background-color: red; width:12px; height:10px; font-size:9px;margin-right:5px;');
                element.innerHTML = '&nbsp;X&nbsp;';
            }
        }
    }
    resolve_links(playdar, results_handler);
}

function resolve_links (playdar, results_handler) {
    var links = unsafeWindow.$$('a');
    for (var i = 0; i < links.length; i++) {
        // Only match links in the /music path
        var url = parseUri(links[i].href);
        if (url.directoryPath.substr(0, 7) != "/music/") {
            continue;
        }
        // Only match track links
        var urlparts = new RegExp(/^\/music\/(.+)\/_\/(.+)/).exec(url.path);
        if (!urlparts) {
            continue;
        }
        // Only match links without a class name (aka virgin links)
        if (links[i].getAttribute('class') != null) {
            continue;
        }
        // Replace + with space
        var artist = urlparts[1].replace(/\+/g, " ");
        var track  = urlparts[2].replace(/\+/g, " ");
        resolve(playdar, links[i], artist, track, results_handler);
    }
}

function throb (qid, link) {
    var throbber = unsafeWindow.document.createElement('span');
    throbber.id = qid;
    throbber.setAttribute('style', 'border:0; margin:0; background-repeat: no-repeat; background-image: url("http://www.playdar.org/static/spinner_10px.gif"); width:12px; height:10px; margin-right:5px;');
    throbber.innerHTML = "&nbsp; &nbsp;";
    link.parentNode.insertBefore(throbber, link);
}

function resolve (playdar, link, artist, track, results_handler) {
    var qid = Playdar.generate_uuid();
    // add a "searching..." status :
    throb(qid, link);
    // register results handler and resolve for this qid
    playdar.register_results_handler(results_handler, qid);
    playdar.resolve(artist, "", track, qid);
}

/* parseUri JS v0.1, by Steven Levithan (http://badassery.blogspot.com)
Splits any well-formed URI into the following parts (all are optional):
----------------------
• source (since the exec() method returns backreference 0 [i.e., the entire match] as key 0, we might as well use it)
• protocol (scheme)
• authority (includes both the domain and port)
    • domain (part of the authority; can be an IP address)
    • port (part of the authority)
• path (includes both the directory path and filename)
    • directoryPath (part of the path; supports directories with periods, and without a trailing backslash)
    • fileName (part of the path)
• query (does not include the leading question mark)
• anchor (fragment)
*/

function parseUri(sourceUri) {
    var uriPartNames = ["source","protocol","authority","domain","port","path","directoryPath","fileName","query","anchor"];
    var uriParts = new RegExp("^(?:([^:/?#.]+):)?(?://)?(([^:/?#]*)(?::(\\d*))?)?((/(?:[^?#](?![^?#/]*\\.[^?#/.]+(?:[\\?#]|$)))*/?)?([^?#/]*))?(?:\\?([^#]*))?(?:#(.*))?").exec(sourceUri);
    var uri = {};
    
    for(var i = 0; i < 10; i++){
        uri[uriPartNames[i]] = (uriParts[i] ? uriParts[i] : "");
    }
    
    // Always end directoryPath with a trailing backslash if a path was present in the source URI
    // Note that a trailing backslash is NOT automatically inserted within or appended to the "path" key
    if(uri.directoryPath.length > 0){
        uri.directoryPath = uri.directoryPath.replace(/\/?$/, "/");
    }
    
    return uri;
}
