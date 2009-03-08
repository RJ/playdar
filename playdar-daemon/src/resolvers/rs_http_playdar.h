#ifndef __RS_HTTP_PLAYDAR_H__
#define __RS_HTTP_PLAYDAR_H__

#include "application/types.h"
#include "resolvers/resolver_service.h"
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "json_spirit/json_spirit.h"
#include <boost/foreach.hpp>

class RS_http_playdar  : public ResolverService
{
    public:
    RS_http_playdar(MyApplication * a, boost::asio::ip::address_v4 ip, unsigned short port) 
        : ResolverService(a)
    {
        m_ip = ip;
        m_port = port;
        
        cout << "Playdar/HTTP resolver online -> " << remote_httpbase() << endl;
    }
    
    
    string remote_httpbase()
    {
        ostringstream s;
        s << "http://" << m_ip.to_string() << ":" << m_port;
        return s.str();
    }

    void start_resolving(boost::shared_ptr<ResolverQuery> rq)
    {
        // A new thread is used for every request in this resolver.
        boost::thread thr(&RS_http_playdar::start_resolving_worker, this, rq);
        // when thr goes out of scope the thread is just detatched, and will
        // eventually finish, hopefully.
    }
    
    void start_resolving_worker(boost::shared_ptr<ResolverQuery> rq)
    {
        query_uid qid = rq->id();
        ostringstream s;
        s << remote_httpbase()
          << "/api/?method=resolve"
          << "&artist=" << esc(rq->artist())
          << "&album="  << esc(rq->album())
          << "&track=" << esc(rq->track())
          << "&qid=" << rq->id()
          ;
        string buf = fetch_http_body( s.str() );
                         
        if( buf.length() )
        {
            using namespace json_spirit;
            Value j;
            if( json_spirit::read( buf, j) ) //parse json response
            {
                Object o = j.get_obj();
                map<string,Value> r;
                obj_to_map(o,r);    
                query_uid remote_qid = r["qid"].get_str();
                //m_qidtable[remote_qid] = qid; //asociate remote qid with our qid
                // check results:
                string check_url = remote_httpbase();
                check_url += "/api/?method=get_results&qid=";
                check_url += remote_qid;
                
                cout << "Playdar/HTTP resolver waiting 1 second before checking for results.." << flush;
                boost::xtime time; 
                boost::xtime_get(&time,boost::TIME_UTC); 
                time.sec += 1;
                boost::thread::sleep(time);
               
                
                buf = fetch_http_body(check_url);
                
                if( buf.length() && /*0 == curl_easy_perform(m_curl) && */ json_spirit::read( buf, j) ) // go!
                {
                    //cout << "curl says ok" << endl;
                    Object ro = j.get_obj();
                    map<string,Value> rr;
                    obj_to_map(ro,rr);    
                    if( rr["qid"].get_str() != remote_qid )
                    {
                        cout << "WTF, these are not the results you are looking for" << endl;
                        return;
                    }else{
                        //cout << "Proceeding.." << endl;
                    }
                    Array resultsA = rr["results"].get_array();
                    vector< boost::shared_ptr<PlayableItem> > v;
                    BOOST_FOREACH(Value & result, resultsA)
                    {
                        Object reso = result.get_obj();
                        boost::shared_ptr<PlayableItem> pip;
                        try
                        {
                            pip = PlayableItem::from_json(reso);
                            // make sure to use whichever ip:port we are actually talking to them via:
                            string url = remote_httpbase();
                            url += "/sid/";
                            url += pip->id();
                            boost::shared_ptr<StreamingStrategy> s(new HTTPStreamingStrategy(url));
                            pip->set_streaming_strategy(s);
                            // get remote pref * out multiplier for speed of network:
                            float  our_pref = (float)0.5*(float)pip->preference();
                            pip->set_preference(our_pref); // "remote" of questionable speed.
                            v.push_back(pip);
                        }
                        catch (...)
                        {
                            cout << "REMOTE_HTTP: Missing fields in response json, discarding" << endl;
                            break;
                        }
                    }
                    report_results(qid, v);
                } else {
                    cout << "httpfail" << endl;
                }
                
                //cout << "Now go and check results from: " << check_url << endl;
                //cout << "parsed json ok" << endl;
            }else{
                cout << "FAIED to parse json" << endl;
            }            
        }else{
            //cout << "fetch_http says fail" << endl;
        }
        cout << "rs_http_playdar thread ending" << endl; 
    }
    
    string esc(const string & s)
    {
        return urlencode(s);
    }
    
    std::string name() { return string("Remote Playdar on ") + remote_httpbase(); }
    
    // nasty hack, "get document body as string from url"
    string fetch_http_body(string url)
    {
        cout << "FETCH "<< url << " ... " << flush;
        ostringstream output;
        try
        {
            HTTPStreamingStrategy h(url);
            char b[512];
            int len = 0;
            //cout <<"Starting to read";
            while((len=h.read_bytes((char *)&b, sizeof(b)))>0)
            {
                //cout << "read:" << len << " " << endl;
                output << string( (char*)&b, len ); 
                //cout << "BUF: " << buf.str << endl;
            }
            cout << "OK" << endl;
            return output.str();
        }
        catch(std::exception e)
        {
            // can't reach server, or some other connection fail
            // TODO granular error handling.
            cout << "FAIL" << endl;
            cout << "*** Caught std::exception " << e.what() << endl;
            return output.str();
        }
    }
    
    protected: 
        ~RS_http_playdar() throw() {}

    private:
        boost::asio::ip::address_v4 m_ip;
        unsigned short m_port;
       // CURL * m_curl;
        
        map<query_uid, query_uid> m_qidtable; // maps remote qid -> local qid
        
        
                
        string urlencode(const string &c)
        {
        
            string escaped="";
            int max = c.length();
            for(int i=0; i<max; i++)
            {
                if ( (48 <= c[i] && c[i] <= 57) ||//0-9
                    (65 <= c[i] && c[i] <= 90) ||//abc...xyz
                    (97 <= c[i] && c[i] <= 122) || //ABC...XYZ
                    (c[i]=='~' || c[i]=='!' || c[i]=='*' || c[i]=='(' || c[i]==')' || c[i]=='\'') //~!*()'
                )
                {
                    escaped.append( &c[i], 1);
                }
                else
                {
                    escaped.append("%");
                    escaped.append( char2hex(c[i]) );//converts char 255 to string "ff"
                }
            }
            return escaped;
        }
        
        string char2hex( char dec )
        {
            char dig1 = (dec&0xF0)>>4;
            char dig2 = (dec&0x0F);
            if ( 0<= dig1 && dig1<= 9) dig1+=48;    //0,48inascii
            if (10<= dig1 && dig1<=15) dig1+=97-10; //a,97inascii
            if ( 0<= dig2 && dig2<= 9) dig2+=48;
            if (10<= dig2 && dig2<=15) dig2+=97-10;
        
            string r;
            r.append( &dig1, 1);
            r.append( &dig2, 1);
            return r;
        }
};


#endif
