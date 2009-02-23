function Playdar() {
    // CALLBACKS YOU SHOULD OVERRIDE:
    this.cb_detected = function(v) { alert('Playdar detected, version: '+v); }
    this.cb_not_detected = function() { alert('Playdar not detected'); }
    this.cb_results_available = function(r,finalanswer) { alert('Results finalanswer: '+finalanswer+' (JSON object): ' + r); }
    // END CALLBACKS

    this.custom_results_handlers = new Array();
    this.qidpoll = new Array();
    this.stat = false;
    this.last_qid="";
    this.libver = "0.1"; // version of playdar.js

    this.init = function() {
        this.stat();
    }

    this.httpbase = function(){
        return "http://localhost:8888"; 
    }

    // api base url for playdar requests
    this.url = function(u){ 
        return this.httpbase() + "/api/?" + u; 
    }

    // turn a source id into an url to stream from
    this.sid2url = function(sid){
        return this.httpbase() + "/sid/" + sid;
    }

    // is playdar installed?
    this.stat = function() {
        setTimeout('playdar.stat_timeout()', 2000);
        this.loadjs(this.url("method=stat&jsonp=playdar.stat_cb"));
    }
    
    this.set_qid_results_callback = function(qid, fun) {
        this.custom_results_handlers[qid]=fun;
    }

    this.stat_timeout = function() { 
        if(this.stat.name != "playdar") this.cb_not_detected();
    }

    this.stat_cb = function(response) {
        if(response.name != "playdar") return false;
        this.stat = response;    
        this.cb_detected(this.stat.version);
    }

    this.resolve_callback = function(response){
        this.last_qid = response.qid;
        this.check_qid(response.qid);
    }

    this.check_qid = function(qid){
        var checkurl = this.url("method=get_results&qid=" + qid + "&jsonp=playdar.search_results");
        this.loadjs(checkurl);
    }

    this.search_results = function(response){
        var finalanswer=false;
        // figure out if we should re-poll, or if the query is solved/failed:
        if( response.query.solved == true ) finalanswer=true;
        if( response.refresh_interval > 0 &&
            !(response.length > 0 && response.results[0].score == 1.0) ){
            if(!this.qidpoll[response.qid]) this.qidpoll[response.qid]=0;
            if(++this.qidpoll[response.qid] < 4){
                setTimeout('playdar.check_qid("'+response.qid+'")', response.refresh_interval);
            }else{
                finalanswer = true; // we won't poll again, tried enough times already
            }
        }
        // now call the results callback:
        if(response.qid && this.custom_results_handlers[response.qid]){
            // custom handler for this specific qid:
            this.custom_results_handlers[response.qid](response, finalanswer);
        }else{
            // standard callback used:
            this.cb_results_available(response, finalanswer);
        }
    }

    this.generate_uuid = function() {
        return "playdar_js_uuid_gen_"+Math.random(); // TODO improve.
    }
    
    // narrow, find an exact match, hopefully
    this.doresolve = function(art,alb,trk,qid){
        var url = "method=resolve";
        url += "&jsonp=playdar.resolve_callback";
        url += "&artist=" + art;
        url += "&album=" + alb;
        url += "&track=" + trk;
        if(typeof(qid)!='undefined') url+="&qid="+qid;
        this.loadjs(this.url(url));
    }

    this.loadjs = function(url){
       var e = document.createElement("script");
       e.src = url;
       e.type="text/javascript";
       document.getElementsByTagName("head")[0].appendChild(e); 
    }

    // format secs -> mm:ss helper.
    this.mmss = function(secs) {
        var s = secs%60;
        if(s < 10) s="0"+s;
        return Math.floor(secs/60) + ":" + s;
    }
} // end Playdar class

var playdar = new Playdar();
