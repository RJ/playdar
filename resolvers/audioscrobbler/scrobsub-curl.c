/***************************************************************************
 *   Copyright 2005-2009 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

/** Really we also want a -KDE and -Gnome option, (for proxy handling and that)
  * but the basic variety should be plain C for portability, so no C++ here 
  * thanks. */

//TODO user-agent

#include "scrobsub.h"
#include <curl/curl.h>
#include <openssl/md5.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

extern void(*scrobsub_callback)(int event, const char* message);
static char* username = "";
static char* session_key = "";
static char* token = "";


void scrobsub_md5(char out[33], const char* in)
{
    MD5_CTX c;
    unsigned char d[MD5_DIGEST_LENGTH];

    MD5_Init(&c);
    MD5_Update(&c, in, strlen(in));
    MD5_Final(d, &c);

    snprintf(out, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
}

const char* scrobsub_username()
{
    return username;
}

const char* scrobsub_session_key()
{
    if (!session_key){
        if (token && !scrobsub_finish_auth()){
            (scrobsub_callback)(SCROBSUB_ERROR_RESPONSE, "Couldn't auth with Last.fm");
            return "";
        }

        if (!username){
            (scrobsub_callback)(SCROBSUB_AUTH_REQUIRED, "");
            return "";
        }
        
        //TODO fetch stored session_key
    }

    return session_key;
}

static int n;

static size_t curl_writer(void* in, size_t size, size_t N, void* out)
{
    size_t x;
    N *= size;
    for (x=0; x<N; ++x){
        if (n-- <= 0) break;
        *out++ = *in++;
    }
    return x;
}

void scrobsub_get(char response[256], const char* url)
{
    n = 256;
    
    CURL* h = curl_easy_init(); //TODO may return NULL
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, curl_writer);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, response);
    CURLcode result = curl_easy_perform(h);
    curl_easy_cleanup(h);
    
    response[255] = '\0'; // curl_writer won't null terminate if >= 256 chars
}

void scrobsub_post(char response[256], const char* url, const char* post_data)
{   
    CURL* h = curl_easy_init(); //TODO may return NULL
    curl_easy_setopt(h, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_perform(h);
    curl_easy_cleanup(h);
    stdcpy(response, "OK\n"); //TODO
}

void scrobsub_auth(char out_url[110])
{
    char url[] = "http://ws.audioscrobbler.com/2.0/?method=auth.gettoken&api_key=" SCROBSUB_API_KEY;
    char response[256];
    scrobsub_get(response, url);
    
    xmlInitParser(); //TODO can't tell if safe to call more than once, but don't want to call unless auth is done to save initialisations
    xmlDocPtr doc = xmlParseMemory(response, 1024); //TODO can return NULL
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj = xmlXPathEvalExpression("/lfm/token/text()", ctx);
    
    token = strdup(obj->stringval);
    
    xmlXPathFreeObject(obj);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    
    strcpy(out_url, "http://www.last.fm/api/auth/?api_key=" SCROBSUB_API_KEY "&token=");
    strcpy(&out_url[38+32+7], token);
}

//TODO localise and get webservice error
//TODO error handling
bool scrobsub_finish_auth()
{
    char sig[9+32+27+32+1] = "api_key" SCROBSUB_API_KEY "methodauth.getSessiontoken%s" SCROBSUB_SHARED_SECRET;
    snprintf(sig, sizeof(sig), sig, token);
    scrobsub_md5(sig, sig);

    char url[66+32+8+32+9+32+1] = "http://ws.audioscrobbler.com/2.0/?method=auth.getSession&api_key=" SCROBSUB_API_KEY "&token=%s&api_sig=%s";
    snprintf(url, sizeof(url), url, token, sig);

    char response[256];
    scrobsub_get(response, url);

    xmlDocPtr doc = xmlParseMemory(response, 256); //TODO can return NULL
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj_key = xmlXPathEvalExpression("/lfm/session/key/text()", ctx);
    xmlXPathObjectPtr obj_name = xmlXPathEvalExpression("/lfm/session/name/text()", ctx);

    session_key = strdup(obj_key->stringval);
    username = strdup(obj_name->stringval);

    xmlXPathFreeObject(obj_key);
    xmlXPathFreeObject(obj_name);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);

    return true;
}
