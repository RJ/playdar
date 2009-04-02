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
#include <CommonCrypto/CommonDigest.h>
#include <Cocoa/Cocoa.h>

static NSString* token;
static NSString* session_key;
extern void(*scrobsub_callback)(int event, const char* message);
bool scrobsub_finish_auth();

void scrobsub_md5(char out[33], const char* in)
{
    unsigned char d[CC_MD5_DIGEST_LENGTH];
	CC_MD5(in, strlen(in), d);
    snprintf(out, sizeof(out), "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
}

bool scrobsub_session_key(const char* out)
{
    if (!session_key)
        return false;
    
    out = [session_key UTF8String];
    return true;
}

static void check_for_session_key()
{
    if (!scrobsub_session_key(NULL)){
        if (!token){
            (scrobsub_callback)(SCROBSUB_AUTH_REQUIRED, "");
            return;
        }
        if (!scrobsub_finish_auth()){
            (scrobsub_callback)(SCROBSUB_ERROR_RESPONSE, "Couldn't auth with Last.fm");
            return;
        }
    }
}    

void scrobsub_get(char response[256], const char* url)
{
    check_for_session_key();

    NSStringEncoding* encoding;
    NSString *output = [NSString stringWithContentsOfURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]
                                            usedEncoding:encoding
                                                   error:nil];
    strncpy(response, [output UTF8String], 256);
}

void scrobsub_post(char response[256], const char* url, const char* post_data)
{   
    //check_for_session_key(); TODO don't think this is required
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]
                                                           cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                       timeoutInterval:10];
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[NSString stringWithUTF8String:post_data]];
//TODO    [request setValue: forHTTPHeaderField:@"User-Agent"];
    
    NSURLResponse* headers;
    NSData* data = [NSURLConnection sendSynchronousRequest:request returningResponse:&headers error:nil];
    
    [data getBytes:response length:256];
}

void scrobsub_auth()
{
    NSURL* url = [NSURL URLWithString:@"http://ws.audioscrobbler.com/2.0/?method=auth.gettoken&api_key=" SCROBSUB_API_KEY ];
    NSXMLDocument* xml = [[NSXMLDocument alloc] initWithContentsOfURL:url options:0 error:nil];
    token = [[[[xml rootElement] elementsForName:@"token"] lastObject] stringValue];
    [token retain];
    url = [NSURL URLWithString:[NSString stringWithFormat:@"http://www.last.fm/api/auth/?api_key=" SCROBSUB_API_KEY "&token=%@", token]];
    [[NSWorkspace sharedWorkspace] openURL:url];
    [xml release];
}

//TODO localise and get webservice error
//TODO store key in keychain
bool scrobsub_finish_auth()
{
    char sig[33];
    NSString* format = @"api_key" SCROBSUB_API_KEY "methodauth.getSessiontoken%@" SCROBSUB_SHARED_SECRET;
    scrobsub_md5(sig, [[NSString stringWithFormat:format, token] UTF8String]);
    NSURL* url = [NSURL URLWithString:[NSString stringWithFormat:@"http://ws.audioscrobbler.com/2.0/?method=auth.getSession&api_key=" SCROBSUB_API_KEY "&token=%@&api_sig=%s", token, sig]];

    NSXMLDocument* xml = [[NSXMLDocument alloc] initWithContentsOfURL:url options:0 error:nil];
    session_key = [[[[xml rootElement] elementsForName:@"key"] lastObject] stringValue];
    NSString* username = [[[[xml rootElement] elementsForName:@"name"] lastObject] stringValue];
    [xml release];
    
    return true;
}
