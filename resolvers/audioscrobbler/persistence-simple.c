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
#include <stdio.h>
#include <string.h>


static FILE* fopen_session_file(const char* mode)
{
    char fn[FILENAME_MAX]; //FILENAME_MAX is as good as we can reasonably go
#if WIN32
    #error Please write the following Windows equivalent, thanks xx
#else
    // In theory, getenv could return NULL, in practice it a) never will, and
    // b) if it does something more serious is already afoot. I prefer more
    // readable code, thanks xx
    strcpy(fn, getenv("HOME"));
#ifdef __APPLE__
    strcat(fn, "/Library/Preferences/fm.last."SCROBSUB_CLIENT_ID".sk");
#else
    const char* xdg = getenv("XDG_CONFIG_PATH");
    if(xdg)
        strcpy(fn, xdg);
    else
        strcat(fn, "/.config");
    strcat(fn, "/Last.fm/"SCROBSUB_CLIENT_ID".sk");
#endif
#endif
    return fopen(fn, mode);
}

bool scrobsub_retrieve_credentials()
{
    FILE* fp = fopen_session_file("r");
    if(!fp) return false;
    scrobsub_session_key = malloc(33);
    fread(scrobsub_session_key, sizeof(char), 32, fp);
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp)-32; //determine length of username
    fseek(fp, 32, SEEK_SET);
    scrobsub_username = malloc(n+1);
    fread(scrobsub_username, sizeof(char), n, fp);
    fclose(fp);

    scrobsub_session_key[32] = '\0';
    scrobsub_username[n] = '\0';

    return true;
}

bool scrobsub_persist_credentials()
{
    FILE* fp = fopen_session_file("w");
    if(!fp) return false;
    fwrite(scrobsub_session_key, sizeof(char), 32, fp);
    fwrite(scrobsub_username, sizeof(char), strlen(scrobsub_username), fp);
    fclose(fp);
    return true;
}
