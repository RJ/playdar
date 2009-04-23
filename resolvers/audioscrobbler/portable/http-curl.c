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
#include <string.h>
#include <curl/curl.h>

//TODO user-agent

static int n;

static size_t curl_writer(void* in, size_t size, size_t N, void* out)
{
    size_t x;
    N *= size;
    for(x=0; x<N; ++x){
        if (n-- <= 0) break;
        *(char*)out++ = *(char*)in++;
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
    
    response[255-n] = '\0'; // curl_writer won't null terminate
}

void scrobsub_post(char response[256], const char* url, const char* post_data)
{   
    CURL* h = curl_easy_init(); //TODO may return NULL
    curl_easy_setopt(h, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_perform(h);
    curl_easy_cleanup(h);
    strcpy(response, "OK\n"); //TODO
}
