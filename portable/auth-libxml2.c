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
