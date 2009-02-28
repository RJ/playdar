Playdar = function (handlers) {
    if (handlers) {
        for (handler in handlers) {
            this.register_handler(handler, handlers[handler]);
        }
    }
    this.uuid = Playdar.generate_uuid();
    Playdar.instances[this.uuid] = this;
};

Playdar.instances = {};
Playdar.generate_uuid = function () {
    return "playdar_js_uuid_gen_" + Math.random(); // TODO improve.
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
   var e = document.createElement("script");
   e.src = url;
   document.getElementsByTagName("head")[0].appendChild(e);
};

Playdar.prototype = {
    lib_version: "0.2",
    server_root: "localhost",
    server_port: "8888",
    stat_timeout: 2000,
    
    register_handler: function (handler_name, callback) {
        this.handlers[handler_name] = callback;
    },
    
    // CALLBACKS YOU SHOULD OVERRIDE:
    handlers: {
        detected: function (v) {
            alert('Playdar detected, version: ' + v);
        },
        not_detected: function () {
            alert('Playdar not detected');
        },
        results: function (r, final_answer) {
            alert('Results final answer: ' + final_answer + ' (JSON object): ' + r);
        }
    },
    // END CALLBACKS
    
    init: function () {
        this.stat();
    },
    
    get_base_url: function (path) {
        var url = "http://" + this.server_root + ":" + this.server_port;
        if (path) {
            url += path;
        }
        return url;
    },
    
    // api base url for playdar requests
    get_url: function (u) {
        return this.get_base_url("/api/?" + u);
    },
    
    jsonp: function (callback) {
        return "&jsonp=Playdar.instances['" + this.uuid + "']." + callback;
    },
    
    // turn a source id into a stream url
    get_stream_url: function (sid) {
        return this.get_base_url("/sid/" + sid);
    },
    
    
    // STAT PLAYDAR INSTALLATION
    
    stat_response: false,
    check_stat: function () {
        return this.stat_response && this.stat_response.name == "playdar";
    },
    
    stat_timeout: function () {
        if (!this.check_stat()) {
            this.handlers.not_detected();
        }
    },
    
    stat_handler: function (response) {
        this.stat_response = response;
        if (!this.check_stat()) {
            this.stat_response = false;
            return false;
        }
        this.handlers.detected(this.stat_response.version);
    },
    
    stat: function () {
        var that = this;
        setTimeout(function () {
            that.stat_timeout();
        }, this.stat_timeout);
        var url = "method=stat"
                + this.jsonp("stat_handler");
        Playdar.loadjs(this.get_url(url));
    },
    
    
    // SEARCH RESULT POLLING
    
    // set search result handlers for a query id
    results_handlers: {},
    register_results_handler: function (qid, handler) {
        this.results_handlers[qid] = handler;
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
        if (response.length > 0 && response.results[0].score == 1.0) {
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
    
    handle_results: function (response) {
        // figure out if we should re-poll, or if the query is solved/failed:
        var final_answer = this.should_stop_polling(response);
        if (!final_answer) {
            var that = this;
            setTimeout(function () {
                that.poll_results(response.qid);
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
    },
    
    // poll results for a query id
    poll_results: function (qid) {
        var url = "method=get_results"
                + "&qid=qid"
                + this.jsonp("handle_results");
        var checkurl = this.get_url(url);
        Playdar.loadjs(checkurl);
    },
    
    
    // CONTENT RESOLUTION
    
    last_qid: "",
    handle_resolution: function (response) {
        this.last_qid = response.qid;
        this.poll_results(response.qid);
    },
    
    // narrow, find an exact match, hopefully
    resolve: function (art, alb, trk, qid) {
        var url = "method=resolve";
                + "&artist=" + art;
                + "&album=" + alb;
                + "&track=" + trk;
                + this.jsonp("handle_resolution");
        if (typeof(qid) != 'undefined') {
            url += "&qid=" + qid;
        }
        Playdar.loadjs(this.get_url(url));
    }
};
