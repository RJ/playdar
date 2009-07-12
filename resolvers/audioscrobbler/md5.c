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
