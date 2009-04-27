/***************************************************************************
 *   Copyright 2009 Last.fm Ltd.                                           *
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

#if __APPLE__
#include <CommonCrypto/CommonDigest.h>
#elif WIN32
#else
#include <openssl/md5.h>
#endif
#include <stdio.h>
#include <string.h>


void scrobsub_md5(char out[33], const char* in)
{
#if WIN32
    #error Pls to implement
#else
    #if __APPLE__
        unsigned char d[CC_MD5_DIGEST_LENGTH];
    	CC_MD5(in, strlen(in), d);
    #else
        MD5_CTX c;
        unsigned char d[MD5_DIGEST_LENGTH];

        MD5_Init(&c);
        MD5_Update(&c, in, strlen(in));
        MD5_Final(d, &c);
    #endif
    snprintf(out, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
             d[0], d[1], d[2],  d[3],  d[4],  d[5],  d[6],  d[7], 
             d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
#endif
}
