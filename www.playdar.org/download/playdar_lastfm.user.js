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
            element.style.backgroundImage = 'none';
            if (response.results.length) {
                // generate tooltip with details:
                var tt = "Sources: ";
                for (var i = 0; i < response.results.length; i++) {
                    var result = response.results[i];
                    tt += result.source + "/" + result.bitrate + "kbps/" + Playdar.mmss(result.duration) + " ";
                }
                // update status element:
                element.style.backgroundColor = "#0a0";
                //var swfstr = "<object height=\"10\" width=\"10\"><embed src=\"http://www.playdar.org/static/player.swf?&song_url=" + playdar.get_stream_url(response.results[0].sid) + "\" height=\"10\" width=\"10\"></embed></object>";
                // Just link to the source, too much flash spam:
                element.innerHTML = "&nbsp;<a href=\"" + playdar.get_stream_url(response.results[0].sid) + "\" title=\"" + tt + "\" style=\"color: #fff;\">" + response.results.length + "</a>&nbsp;";
            } else {
                element.style.backgroundColor = "#c00";
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
        var path = links[i].pathname;
        if (path.substr(0, 7) != "/music/") {
            continue;
        }
        // Only match track links
        var urlparts = new RegExp(/^\/music\/(.+)\/_\/(.+)/).exec(path);
        if (!urlparts) {
            continue;
        }
        // Only match links without a class name (aka virgin links)
        if (links[i].getAttribute('class') != null) {
            continue;
        }
        // Don't match image links
        if (links[i].firstChild.nodeName.toUpperCase() == "IMG") {
            continue;
        }
        // Replace + with space
        var artist = urlparts[1].replace(/\+/g, " ");
        var track  = urlparts[2].replace(/\+/g, " ");
        resolve(playdar, links[i], artist, track, results_handler);
    }
}

function start_status (qid, link) {
    var status = unsafeWindow.document.createElement('span');
    status.id = qid;
    status.style.cssFloat = "right";
    status.style.display = "block";
    status.style.border = 0;
    status.style.margin = "0 5px 0 0";
    status.style.backgroundRepeat = "no-repeat";
    status.style.backgroundImage = 'url("http://www.playdar.org/static/spinner_10px.gif")';
    status.style.color = "#fff";
    status.style.width = "13px";
    status.style.height = "13px";
    status.style.textAlign = "center";
    status.style.fontSize = "9px";
    status.innerHTML = "&nbsp; &nbsp;";
    var parent = link.parentNode;
    parent.insertBefore(status, parent.firstChild);
}

function resolve (playdar, link, artist, track, results_handler) {
    var qid = Playdar.generate_uuid();
    // add a "searching..." status :
    start_status(qid, link);
    // register results handler and resolve for this qid
    playdar.register_results_handler(results_handler, qid);
    playdar.resolve(artist, "", track, qid);
}