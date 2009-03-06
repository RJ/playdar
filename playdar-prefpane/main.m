/*
 Created on 28/02/2009
 Copyright 2009 Max Howell <mxcl@users.sf.net>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//TODO the daemon needs be named the same as cmake builds it, or conflicts with 
// source users occur, xcode hates you for this though

//TODO watch exit code of playdard (if early exits), and watch process while pref pane is open
// ^^ important in case playdard exits immediately due to crash or somesuch

//TODO remember path that we scanned with defaults controller
//TODO memory leaks
//TODO log that stupid exception
//TODO sparkle updates
// ^^ ensure if prefpane is updated, it restarts playdard
//TODO start at login checkbox


#import "main.h"
#include <sys/sysctl.h>
#include <Sparkle/SUUpdater.h>
#include "LoginItemsAE.h"


/** returns the pid of the running playdard instance, or 0 if not found */
static pid_t playdard_pid()
{
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    struct kinfo_proc *info;
    size_t N;
    pid_t pid = 0;
    
    if(sysctl(mib, 3, NULL, &N, NULL, 0) < 0)
        return 0; //wrong but unlikely
    if(!(info = NSZoneMalloc(NULL, N)))
        return 0; //wrong but unlikely
    if(sysctl(mib, 3, info, &N, NULL, 0) < 0)
        goto end;

    N = N / sizeof(struct kinfo_proc);
    for(size_t i = 0; i < N; i++)
        if(strcmp(info[i].kp_proc.p_comm, "playdar") == 0)
            { pid = info[i].kp_proc.p_pid; break; }
end:
    NSZoneFree(NULL, info);
    return pid;
}

static inline NSString* ini_path()
{
    return [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/org.playdar.ini"];
}

static inline NSString* db_path()
{
    return [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/Playdar/collection.db"];
}

static inline NSString* fullname()
{
    // being cautious
    NSString* fullname = NSFullUserName();
    return (fullname && [fullname length] > 0) ? fullname : NSUserName();
}

#define PLAYDAR_BIN_PATH [[[self bundle] bundlePath] stringByAppendingPathComponent:@"Contents/MacOS/playdar"]


@implementation OrgPlaydarPreferencePane

-(void)mainViewDidLoad
{
    [[SUUpdater updaterForBundle:[self bundle]] resetUpdateCycle];
    
    NSString* ini = ini_path();
    if([[NSFileManager defaultManager] fileExistsAtPath:ini] == false) 
    {
        NSArray* args = [NSArray arrayWithObjects: fullname(), db_path(), ini, nil];
        [self execScript:@"playdar.ini.rb" withArgs:args];
    }

    [[popup menu] addItem:[NSMenuItem separatorItem]];
    [[[popup menu] addItemWithTitle:@"Select..." action:@selector(select:) keyEquivalent:@""] setTarget:self];

    NSString* home = NSHomeDirectory();
    [self addFolder:[home stringByAppendingPathComponent:@"Music"] setSelected:true];
    [self addFolder:home setSelected:false];
        
    if(pid = playdard_pid()) [start setTitle:@"Stop Playdar"];
    if(![self isLoginItem]) [check setState:NSOffState];
}

-(void)addFolder:(NSString*)path setSelected:(bool)select
{
    int const index = [[popup menu] numberOfItems]-2;
    NSMenuItem* item = [[popup menu] insertItemWithTitle:path action:nil keyEquivalent:@"" atIndex:index];
    NSImage* image = [[NSWorkspace sharedWorkspace] iconForFileType:NSFileTypeForHFSTypeCode(kGenericFolderIcon)];
    [image setSize:NSMakeSize(16, 16)];
    [item setImage:image];
    if(select) [popup selectItemAtIndex:index];
}
 
-(void)onScan:(id)sender
{
    NSArray* args = [NSArray arrayWithObjects:[popup titleOfSelectedItem], db_path(), nil];
    [self execScript:@"scanner.sh" withArgs:args];
}

-(void)onStart:(id)sender
{
    NSMutableArray* args = [NSArray arrayWithObjects:@"-c", ini_path(), nil];
    
    if(pid) {
        if(kill( pid, SIGKILL ) != 0) return;
        [start setTitle:@"Start Playdar"];
        pid = 0;
    } 
    else if([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
        [self execScript:@"playdar.sh" withArgs:args];
    else {
        NSString* path = PLAYDAR_BIN_PATH;
        @try
        {
            NSTask *task = [[NSTask alloc] init];
            [task setLaunchPath:path];
            [task setArguments:args];
            [task launch];
            [start setTitle:@"Stop Playdar"];
            pid = [task processIdentifier];
        }
        @catch(NSException* e)
        {
            NSString* msg = @"The file at \"";
            msg = [msg stringByAppendingString:path];
            msg = [msg stringByAppendingString:@"\" could not be executed."];
            
            NSBeginAlertSheet(@"Could not start Playdar.", 
                              nil, nil, nil,
                              [[self mainView] window],
                              self,
                              nil, nil,
                              nil,
                              msg );
        }
    }
}

-(void)onStartAtLogin:(id)sender
{
    bool const enabled = [check state] == NSOnState;
    
	CFArrayRef loginItems = NULL;
	NSURL *url = [NSURL fileURLWithPath:PLAYDAR_BIN_PATH];
	int existingLoginItemIndex = -1;
    
    NSLog( @"%@", url );
    
	OSStatus status = LIAECopyLoginItems(&loginItems);
    
	if(status == noErr) {
		NSEnumerator *enumerator = [(NSArray *)loginItems objectEnumerator];
		NSDictionary *loginItemDict;
        
		while((loginItemDict = [enumerator nextObject])) {
			if([[loginItemDict objectForKey:(NSString *)kLIAEURL] isEqual:url]) {
				existingLoginItemIndex = [(NSArray *)loginItems indexOfObjectIdenticalTo:loginItemDict];
				break;
			}
		}
	}
    
	if(enabled && (existingLoginItemIndex == -1))
		LIAEAddURLAtEnd((CFURLRef)url, false);
	else if(!enabled && (existingLoginItemIndex != -1))
		LIAERemove(existingLoginItemIndex);
    
	if(loginItems)
		CFRelease(loginItems);
}

-(bool)isLoginItem
{
    Boolean foundIt = false;
    CFArrayRef loginItems = NULL;
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)PLAYDAR_BIN_PATH, kCFURLPOSIXPathStyle, false);
    NSLog( @"%@", url );
    OSStatus status = LIAECopyLoginItems(&loginItems);
    if(status == noErr) {
        for(CFIndex i=0, N=CFArrayGetCount(loginItems); i<N; ++i) {
            CFDictionaryRef loginItem = CFArrayGetValueAtIndex(loginItems, i);
            foundIt = CFEqual(CFDictionaryGetValue(loginItem, kLIAEURL), url);
            if(foundIt) break;
        }
        CFRelease(loginItems);
    }
    CFRelease(url);  
    return foundIt;
}

////// Directory selector
-(void)select:(id)sender
{
    // return if not the Select... item
    if([popup indexOfSelectedItem] != [popup numberOfItems]-1) return;
    
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:NO];
    [panel setCanChooseDirectories:YES];
    [panel beginSheetForDirectory:nil 
                             file:nil 
                   modalForWindow:[[self mainView] window]
                    modalDelegate:self
                   didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                      contextInfo:nil];
}

-(void)openPanelDidEnd:(NSOpenPanel*)panel
            returnCode:(int)returnCode
           contextInfo:(void*)contextInfo 
{
    if(returnCode == NSOKButton) {
        [self addFolder:[panel filename] setSelected:true];
    } else
        [popup selectItemAtIndex:0];
}
////// Directory selector

-(void)execScript:(NSString*)script_name withArgs:(NSArray*)args
{
    @try {
        NSString* resources = [[self bundle] resourcePath];
        NSString* path = [resources stringByAppendingPathComponent:script_name];
        
        NSTask *task = [[NSTask alloc] init];
        [task setCurrentDirectoryPath:resources];
        [task setLaunchPath:path];
        [task setArguments:args];
        [task launch];
    }
    @catch (NSException* e)
    {
        //TODO log - couldn't figure out easy way to do this
    }
}

@end
