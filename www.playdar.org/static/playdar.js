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
   var e = document.createElement("script");
   e.src = url;
   document.getElementsByTagName("head")[0].appendChild(e);
};

Playdar.prototype = {
    lib_version: "0.2",
    server_root: "localhost",
    server_port: "8888",
    stat_timeout: 2000,
    
    // CALLBACK FUNCTIONS REGISTERED AT CONSTRUCTION
    
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
    
    register_handler: function (handler_name, callback) {
        this.handlers[handler_name] = callback;
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
        Playdar.loadjs(this.get_url("stat", "stat_handler"));
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
        Playdar.loadjs(this.get_url("get_results", "handle_results", {
            qid: qid
        }));
    },
    
    
    // CONTENT RESOLUTION
    
    last_qid: "",
    handle_resolution: function (response) {
        this.last_qid = response.qid;
        this.poll_results(response.qid);
    },
    
    // narrow, find an exact match, hopefully
    resolve: function (art, alb, trk, qid) {
        params = {
            artist: art,
            album: alb,
            track: trk
        };
        if (typeof qid !== 'undefined') {
            params.qid = qid;
        }
        Playdar.loadjs(this.get_url("resolve", "handle_resolution", params));
    }
};
