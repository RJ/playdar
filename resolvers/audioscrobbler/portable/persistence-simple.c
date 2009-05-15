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
#include <stdio.h>
#include <string.h>

#include <stdlib.h> // needed for getenv() on linux


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
