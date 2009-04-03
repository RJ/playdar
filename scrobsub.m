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

#define KEYCHAIN_NAME "fm.last.Audioscrobbler"

static NSString* token;
static NSString* session_key;
static NSString* username;
extern void(*scrobsub_callback)(int event, const char* message);
bool scrobsub_finish_auth();

void scrobsub_md5(char out[33], const char* in)
{
    unsigned char d[CC_MD5_DIGEST_LENGTH];
	CC_MD5(in, strlen(in), d);
    snprintf(out, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
}

const char* scrobsub_username()
{
    return username ? [username UTF8String] : 0;
}

const char* scrobsub_session_key()
{
    if (!session_key){
        if (token && !scrobsub_finish_auth()){
            (scrobsub_callback)(SCROBSUB_ERROR_RESPONSE, "Couldn't auth with Last.fm");
            return 0;
        }

        username = [[NSUserDefaults standardUserDefaults] stringForKey:@"Username"];
        const char* utf8_username = [username UTF8String];
        if (!utf8_username){
            (scrobsub_callback)(SCROBSUB_AUTH_REQUIRED, "");
            return 0;
        }
        
        void* key;
        UInt32 n;
        OSStatus err = SecKeychainFindGenericPassword(NULL, //default keychain
                                                      sizeof(KEYCHAIN_NAME),
                                                      KEYCHAIN_NAME,
                                                      strlen(utf8_username),
                                                      utf8_username,
                                                      &n,
                                                      &key,
                                                      NULL);
        session_key = [[NSString alloc] initWithBytes:key length:n encoding:NSASCIIStringEncoding];
        SecKeychainItemFreeContent(NULL, key);
        (void)err; //TODO
    }
    return [session_key UTF8String];
}

void scrobsub_get(char response[256], const char* url)
{
    NSStringEncoding encoding;
    NSString *output = [NSString stringWithContentsOfURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]
                                            usedEncoding:&encoding
                                                   error:nil];
    strncpy(response, [output UTF8String], 256);
}

void scrobsub_post(char response[256], const char* url, const char* post_data)
{   
    NSURL* urls = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:urls
                                                           cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                       timeoutInterval:10];
    
    NSData *postData = [[NSString stringWithUTF8String:post_data] dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:YES];
    NSString *postLength = [NSString stringWithFormat:@"%d", [postData length]];
    
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:postData];
    [request setValue:@"ARSE" forHTTPHeaderField:@"User-Agent"];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];

    [[request URL] retain]; //debug TODO remove
    [request retain]; //debug TODO remove
    
    NSURLResponse* headers = NULL;
    NSError* error = NULL;
    NSData* data = [NSURLConnection sendSynchronousRequest:request returningResponse:&headers error:&error];
    
    [data getBytes:response length:256];
}

void scrobsub_auth()
{
    if (token) return;
    
    NSURL* url = [NSURL URLWithString:@"http://ws.audioscrobbler.com/2.0/?method=auth.gettoken&api_key=" SCROBSUB_API_KEY ];
    NSXMLDocument* xml = [[NSXMLDocument alloc] initWithContentsOfURL:url options:0 error:nil];
    token = [[[[xml rootElement] elementsForName:@"token"] lastObject] stringValue];
    [token retain];
    url = [NSURL URLWithString:[NSString stringWithFormat:@"http://www.last.fm/api/auth/?api_key=" SCROBSUB_API_KEY "&token=%@", token]];
    [[NSWorkspace sharedWorkspace] openURL:url];
    [xml release];
}


//TODO localise and get webservice error
//TODO error handling
bool scrobsub_finish_auth()
{
    char sig[33];
    NSString* format = @"api_key" SCROBSUB_API_KEY "methodauth.getSessiontoken%@" SCROBSUB_SHARED_SECRET;
    scrobsub_md5(sig, [[NSString stringWithFormat:format, token] UTF8String]);
    NSURL* url = [NSURL URLWithString:[NSString stringWithFormat:@"http://ws.audioscrobbler.com/2.0/?method=auth.getSession&api_key=" SCROBSUB_API_KEY "&token=%@&api_sig=%s", token, sig]];

    NSXMLDocument* xml = [[NSXMLDocument alloc] initWithContentsOfURL:url options:0 error:nil];
    NSXMLElement* session = [[[xml rootElement] elementsForName:@"session"] lastObject];
    session_key = [[[session elementsForName:@"key"] lastObject] stringValue];
    username = [[[session elementsForName:@"name"] lastObject] stringValue];
    [xml release];
    [session_key retain];
    [username retain];
    
    const char* utf8_username = [username UTF8String];
    OSStatus err = SecKeychainAddGenericPassword(NULL, //default keychain
                                                 sizeof(KEYCHAIN_NAME),
                                                 KEYCHAIN_NAME,
                                                 strlen(utf8_username),
                                                 utf8_username,
                                                 32,
                                                 [session_key UTF8String],
                                                 NULL);
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:username forKey:@"Username"];
    [defaults synchronize];

    (void)err; //TODO
    
    return true;
}
