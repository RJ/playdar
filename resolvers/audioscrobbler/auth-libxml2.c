/*
   Copyright 2009 Last.fm Ltd.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   
   This file was originally created by Max Howell <max@last.fm>
*/
#include "scrobsub.h"
#include <libxml/parser.h>
#include <libxml/xpath.h>

bool scrobsub_persist_credentials();
static char* token = 0;

//TODO localise and get webservice error
//TODO error handling

void scrobsub_auth(char out_url[110])
{
    char url[] = "http://ws.audioscrobbler.com/2.0/?method=auth.gettoken&api_key=" SCROBSUB_API_KEY;
    char response[256];
    scrobsub_get(response, url);

    xmlInitParser(); //TODO can't tell if safe to call more than once, but don't want to call unless auth is done to save initialisations
    xmlDocPtr doc = xmlParseMemory(response, strlen(response)); //TODO can return NULL
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj = xmlXPathEvalExpression("/lfm/token/text()", ctx);
    
    if(!obj) return; // leak, but I don't care, this is an unlikely route

    token = strdup(obj->nodesetval->nodeTab[0]->content);
    strcpy(out_url, "http://www.last.fm/api/auth/?api_key=" SCROBSUB_API_KEY "&token=");
    strcat(out_url, token);
    
    xmlXPathFreeObject(obj);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
}

bool scrobsub_finish_auth()
{
    if(!token) return false; // developer needs to call scrobsub_auth() first
    if(scrobsub_session_key) return true; // auth already complete
    
    char sig[7+32+26+32+32+1];
    snprintf(sig, sizeof(sig), "api_key" SCROBSUB_API_KEY "methodauth.getSessiontoken%s" SCROBSUB_SHARED_SECRET, token);
    scrobsub_md5(sig, sig);

    char url[65+32+7+32+9+32+1];
    snprintf(url, sizeof(url), "http://ws.audioscrobbler.com/2.0/?method=auth.getSession&api_key=" SCROBSUB_API_KEY "&token=%s&api_sig=%s", token, sig);

    char response[256];
    scrobsub_get(response, url);

    xmlDocPtr doc = xmlParseMemory(response, 256); //TODO can return NULL
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj_key = xmlXPathEvalExpression("/lfm/session/key/text()", ctx);
    xmlXPathObjectPtr obj_name = xmlXPathEvalExpression("/lfm/session/name/text()", ctx);

    if(obj_key) scrobsub_session_key = strdup(obj_key->nodesetval->nodeTab[0]->content);
    if(obj_name) scrobsub_username = strdup(obj_name->nodesetval->nodeTab[0]->content);

    xmlXPathFreeObject(obj_key);
    xmlXPathFreeObject(obj_name);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    
    return scrobsub_persist_credentials() && scrobsub_session_key && scrobsub_username;
}
