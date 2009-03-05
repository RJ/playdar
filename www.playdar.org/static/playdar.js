Playdar = function (handlers) {
    for (handler in this.handlers) {
        this.register_handler(handler, this.handlers[handler]);
    }
    if (handlers) {
        for (handler in handlers) {
            this.register_handler(handler, handlers[handler]);
        }
    }
    this.uuid = Playdar.generate_uuid();
    Playdar.last = this;
    Playdar.instances[this.uuid] = Playdar.last;
};

Playdar.last = null;
Playdar.instances = {};
Playdar.create = function (handlers) {
    return new Playdar(handlers);
};

/*
Based on: Math.uuid.js
Version: 1.3
Latest version:   http://www.broofa.com/Tools/Math.uuid.js
Information:      http://www.broofa.com/blog/?p=151
Contact:          robert@broofa.com
----
Copyright (c) 2008, Robert Kieffer
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Robert Kieffer nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
Playdar.generate_uuid = function () {
    // Private array of chars to use
    var chars = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'.split('');
    var uuid = [];
    var rnd = Math.random;
    
    // rfc4122, version 4 form
    var r;
    
    // rfc4122 requires these characters
    uuid[8] = uuid[13] = uuid[18] = uuid[23] = '-';
    uuid[14] = '4';
    
    // Fill in random data.  At i==19 set the high bits of clock sequence as
    // per rfc4122, sec. 4.1.5
    for (var i = 0; i < 36; i++) {
        if (!uuid[i]) {
            r = 0 | rnd()*16;
            uuid[i] = chars[(i == 19) ? (r & 0x3) | 0x8 : r & 0xf];
        }
    }
    return uuid.join('');
};

Playdar.toQueryString = function (params) {
    function toQueryPair(key, value) {
        if (value === null) {
            return key;
        }
        return key + '=' + encodeURIComponent(value);
    }
    
    var results = [];
    for (key in params) {
        var values = params[key];
        key = encodeURIComponent(key);
        
        if (Object.prototype.toString.call(values) == '[object Array]') {
            for (i = 0; i < values.length; i++) {
                results.push(toQueryPair(key, values[i]));
            }
        } else {
            results.push(toQueryPair(key, values));
        }
    }
    return results.join('&');
};

// format secs -> mm:ss helper.
Playdar.mmss = function (secs) {
    var s = secs % 60;
    if (s < 10) {
        s = "0" + s;
    }
    return Math.floor(secs/60) + ":" + s;
};
    
Playdar.loadjs = function (url) {
   var s = document.createElement("script");
   // console.info('loadjs:', url);
   s.src = url;
   document.getElementsByTagName("head")[0].appendChild(s);
};

Playdar.status_bar = null;

Playdar.prototype = {
    lib_version: "0.3",
    server_root: "localhost",
    server_port: "8888",
    stat_timeout: 2000,
    web_host: "http://www.playdar.org",
    
    show_status: function (text, bg) {
        if (!bg) {
            var bg = "cbdab1";
        }
        if (!Playdar.status_bar) {
            Playdar.status_bar = document.createElement("div");
            Playdar.status_bar.style.position = 'fixed';
            Playdar.status_bar.style.bottom = 0;
            Playdar.status_bar.style.left = 0;
            Playdar.status_bar.style.width = '100%';
            Playdar.status_bar.style.textIndent = '1px';
            Playdar.status_bar.style.borderTop = '1px solid #bbb';
            Playdar.status_bar.style.color = '#000';
            Playdar.status_bar.style.font = 'normal 12px "Verdana", sans-serif';
        }
        Playdar.status_bar.style.background = '#' + bg;
        
        if (!this.status_message) {
            this.status_message = document.createElement("p");
            this.status_message.style.padding = "7px";
            this.status_message.style.margin = "0";
            Playdar.status_bar.appendChild(this.status_message);
        }
        this.status_message.innerHTML = text;
        
        if (!this.query_count) {
            this.query_count = document.createElement("span");
            this.query_count.style.cssFloat = "right";
            this.query_count.style.margin = "7px";
            Playdar.status_bar.insertBefore(this.query_count, Playdar.status_bar.firstChild);
        }
        
        document.body.appendChild(Playdar.status_bar);
    },
    
    // CALLBACK FUNCTIONS REGISTERED AT CONSTRUCTION
    
    handlers: {
        detected: function (version) {
            this.detected_version = version;
            this.show_detected_message();
        },
        not_detected: function () {
            this.show_status("Playdar not detected.", 'F0D3C3');
        },
        stat_complete: function (detected) {
            return detected;
        },
        results: function (response, final_answer) {
            if (final_answer) {
                if (response.results.length) {
                    alert('Found results: ' + response.results.length);
                } else {
                    alert('No results');
                }
            }
        }
    },
    
    register_handler: function (handler_name, callback) {
        if (!callback) {
            var callback = function () {};
        }
        var playdar = this;
        this.handlers[handler_name] = function () { return callback.apply(playdar, arguments); };
    },
    
    show_detected_message: function () {
        var messages = [];
        if (this.detected_version) {
            messages.push('<a href="' + this.web_host + '"><img src="' + this.web_host + '/static/playdar_logo_16x16.png" width="16" height="16" style="vertical-align: middle; float: left; margin: 0 5px 0 0; border: 0;" /> Playdar detected</a>. Version: ' + this.detected_version);
        }
        if (this.soundmanager) {
            messages.push('<a href="http://schillmania.com/projects/soundmanager2/">Soundmanager registered</a> (' + this.soundmanager.versionNumber + ')');
        }
        this.show_status(messages.join(' | '));
    },
    
    // initialisation
    init: function () {
        this.stat();
    },
    
    // UTILITY FUNCTIONS
    
    get_base_url: function (path) {
        var url = "http://" + this.server_root + ":" + this.server_port;
        if (path) {
            url += path;
        }
        return url;
    },
    
    // build an api url for playdar requests
    get_url: function (method, jsonp, options) {
        if (!options) {
            options = {};
        }
        options.method = method;
        options.jsonp = this.jsonp_callback(jsonp);
        return this.get_base_url("/api/?" + Playdar.toQueryString(options));
    },
    
    // turn a source id into a stream url
    get_stream_url: function (sid) {
        return this.get_base_url("/sid/" + sid);
    },
    
    // build the jsonp callback string
    jsonp_callback: function (callback) {
        return "Playdar.instances['" + this.uuid + "']." + callback;
    },
    
    list_results: function (response) {
        for (var i = 0; i < response.results.length; i++) {
            console.log(response.results[i].name);
        }
    },
    
    
    // STAT PLAYDAR INSTALLATION
    
    stat_response: false,
    check_stat: function () {
        return this.stat_response && this.stat_response.name == "playdar";
    },
    
    check_stat_timeout: function () {
        if (!this.check_stat()) {
            this.handlers.not_detected();
            this.handlers.stat_complete(false);
        }
    },
    
    handle_stat: function (response) {
        // console.dir(response);
        this.stat_response = response;
        if (!this.check_stat()) {
            this.stat_response = false;
            return false;
        }
        this.handlers.detected(this.stat_response.version);
        this.handlers.stat_complete(true);
    },
    
    stat: function () {
        var playdar = this;
        setTimeout(function () {
            playdar.check_stat_timeout();
        }, this.stat_timeout);
        Playdar.loadjs(this.get_url("stat", "handle_stat"));
    },
    
    
    // SEARCH RESULT POLLING
    
    // set search result handlers for a query id
    results_handlers: {},
    register_results_handler: function (handler, qid) {
        if (qid) {
            this.results_handlers[qid] = handler;
        } else {
            this.register_handler('results', handler);
        }
    },
    
    poll_counts: {},
    should_stop_polling: function (response) {
        // Stop if we've exceeded our refresh limit
        if (response.refresh_interval <= 0) {
            return true;
        }
        // Stop if the query is solved
        if (response.query.solved == true) {
            return true;
        }
        // Stop if we've got a perfect match
        if (response.results.length && response.results[0].score == 1.0) {
            return true;
        }
        // Stop if we've exceeded 4 polls
        if (!this.poll_counts[response.qid]) {
            this.poll_counts[response.qid] = 0;
        }
        if (++this.poll_counts[response.qid] >= 4) {
            return true;
        }
        return false;
    },
    
    success_count: 0,
    handle_results: function (response) {
        // console.dir(response);
        // figure out if we should re-poll, or if the query is solved/failed:
        var final_answer = this.should_stop_polling(response);
        if (!final_answer) {
            var playdar = this;
            setTimeout(function () {
                playdar.get_results(response.qid);
            }, response.refresh_interval);
        }
        // now call the results handler
        if (response.qid && this.results_handlers[response.qid]) {
            // try a custom handler registered for this query id
            this.results_handlers[response.qid](response, final_answer);
        } else {
            // fall back to standard handler
            this.handlers.results(response, final_answer);
        }
        if (final_answer) {
            this.pending_count--;
            if (response.results.length) {
                this.success_count++;
            }
        }
        
        this.show_resolution_status();
    },
    
    // poll results for a query id
    get_results: function (qid) {
        Playdar.loadjs(this.get_url("get_results", "handle_results", {
            qid: qid
        }));
    },
    
    get_last_results: function () {
        if (this.last_qid) {
            this.increment_requests();
            this.get_results(this.last_qid);
        }
    },
    
    // CONTENT RESOLUTION
    
    resolve_qids: [],
    last_qid: "",
    
    
    increment_requests: function () {
        this.request_count++;
        this.pending_count++;
        this.show_resolution_status();
    },
    
    show_resolution_status: function () {
        if (this.query_count) {
            var status = "";
            if (this.pending_count) {
                status += 'Searching: ' + this.pending_count + ' <img src="' + this.web_host + '/static/spinner_10px.gif" width="10" height="10"/> ';
            }
            status += "Resolved: " + this.success_count + "/" + this.request_count;
            this.query_count.innerHTML = status;
        }
    },
    
    handle_resolution: function (response) {
        // console.dir(response);
        this.last_qid = response.qid;
        this.resolve_qids.push(this.last_qid);
        this.get_results(response.qid);
    },
    
    request_count: 0,
    pending_count: 0,
    resolve: function (art, alb, trk, qid) {
        params = {
            artist: art,
            album: alb,
            track: trk
        };
        if (typeof qid !== 'undefined') {
            params.qid = qid;
        }
        this.increment_requests();
        Playdar.loadjs(this.get_url("resolve", "handle_resolution", params));
    },
    
    // STREAMING WITH SOUNDMANAGER
    
    soundmanager: null,
    
    register_stream: function (sid, options) {
        if (!this.soundmanager) {
            return false;
        }
        if (!options) {
            var options = {};
        }
        options.id = sid;
        options.url = this.get_stream_url(sid);
        if (this.soundmanager) {
            return this.soundmanager.createSound(options);
        }
    },
    play_stream: function (sid) {
        if (!this.soundmanager) {
            return false;
        }
        var sound = this.soundmanager.sounds[sid];
        sound.togglePause();
        return sound;
    }
};
