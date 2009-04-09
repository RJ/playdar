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

#define SCROBSUB_NO_RELAY 1
#define SCROBSUB_CLIENT_ID "ass"

#include "_configure.h"
#include "scrobsub.c"
#include "scrobsub-curl.c"
#include <unistd.h>


static void callback(int e, const char* s)
{
    if (e == SCROBSUB_AUTH_REQUIRED)
    {
        char url[110];
        scrobsub_auth(url);
        printf("Please visit\n%s\nto authorize scrobbling and then press enter to continue.\n", url);
        char c = getc(stdin);
    }
}


int main()
{
    scrobsub_init(callback);
    scrobsub_start("Max Howell", "Is Fucking Awesome", "Audioscrobbler EXAMPLE", 60, 1, "");
    printf("Fake track is playing for 60 seconds...\n");
    sleep(60);
    scrobsub_stop();
    return 0;
}
