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
#include <string.h>
#include <curl/curl.h>

//TODO user-agent

static int n;

static size_t curl_writer(void* in, size_t size, size_t N, void* out)
{
    size_t x;
    N *= size;
    for(x=0; x<N; ++x){
        *(char*)out++ = *(char*)in++;
        if (++n == 255) break;
    }
    return x;
}

void scrobsub_get(char response[256], const char* url)
{
    n = 0;
    
    CURL* h = curl_easy_init(); //TODO may return NULL
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, curl_writer);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, response);
    CURLcode result = curl_easy_perform(h);
    curl_easy_cleanup(h);
    
    response[n] = '\0'; // curl_writer won't null terminate
}

void scrobsub_post(char response[256], const char* url, const char* post_data)
{
    n = 0;

    CURL* h = curl_easy_init(); //TODO may return NULL
    curl_easy_setopt(h, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, curl_writer);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, response);
    curl_easy_perform(h);
    curl_easy_cleanup(h);

    response[n] = '\0'; // curl_writer won't null terminate
}
