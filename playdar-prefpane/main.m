/*
 Created on 28/02/2009
 Copyright 2009 Max Howell <max@methylblue.com>
 
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

// Please forgive my n00bness at Cocoa.
// I'd actually appreciate it if you emailed me my mistakes! Thanks.

//TODO when launching, watch the NSTask, and say "Crashed :(" if early exit
//TODO otherwise check status of pid when window becomes key and update button
//TODO remember path that we scanned with defaults controller
//TODO memory leaks
//TODO log that stupid exception
//TODO while open auto restart playdar binary if using byo_bin and playdar binary is modified
//TODO defaults should use org.playdar.plist not com.apple.systempreferences.plist
//TODO animate in a link to the demo page when user runs the app and it works
//TODO pipe output to a log file

#import "main.h"
#include "LoginItemsAE.h"
#include <Sparkle/SUUpdater.h>
#include <sys/sysctl.h>


/** returns the pid of the running playdar instance, or 0 if not found */
static pid_t playdar_pid()
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
    return [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/org.playdar.json"];
}

static inline NSString* db_path()
{
    return [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/Playdar/collection.db"];
}

static inline NSString* daemon_script_path()
{
    return [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/Playdar/playdar.command"];
}

static inline NSString* fullname()
{
    // being cautious
    NSString* fullname = NSFullUserName();
    return (fullname && [fullname length] > 0) ? fullname : NSUserName();
}



@implementation OrgPlaydarPreferencePane

-(void)mainViewDidLoad
{   
    NSFileManager* fm = [NSFileManager defaultManager];
    if([fm fileExistsAtPath:daemon_script_path()] == false)
        [self writeDaemonScript];
    NSString* ini = ini_path();
    if([fm fileExistsAtPath:ini] == false){
        NSArray* args = [NSArray arrayWithObjects: fullname(), db_path(), ini, nil];
        [self execScript:@"playdar.conf.rb" withArgs:args];
    }

    [[popup menu] addItem:[NSMenuItem separatorItem]];
    [[[popup menu] addItemWithTitle:@"Other..." action:@selector(onSelect:) keyEquivalent:@""] setTarget:self];

    NSString* home = NSHomeDirectory();
    [self addFolder:[home stringByAppendingPathComponent:@"Music"] setSelected:true];
    [self addFolder:home setSelected:false];

    pid = playdar_pid();
    if(pid){
        [enable setState:NSOnState];
        [demos setHidden:false];
        [info setHidden:false];
        NSSize size = [[self mainView] frame].size;
        size.height += 20;
        [[self mainView] setFrameSize:size];
    }

////// Sparkle
    SUUpdater* updater = [SUUpdater updaterForBundle:[self bundle]];
    [updater resetUpdateCycle];
    [updater setDelegate:self];

    if([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
        [updater checkForUpdates:self];
}

-(void)addFolder:(NSString*)path setSelected:(bool)select
{
    int const index = [[popup menu] numberOfItems]-2;
    NSMenuItem* item = [[popup menu] insertItemWithTitle:path action:nil keyEquivalent:@"" atIndex:index];
    NSImage* image = [[NSWorkspace sharedWorkspace] iconForFile:path];
    [image setSize:NSMakeSize(16, 16)];
    [item setImage:image];
    if(select) [popup selectItemAtIndex:index];
}
 
-(void)onScan:(id)sender
{
    NSArray* args = [NSArray arrayWithObjects:[popup titleOfSelectedItem], db_path(), nil];
    [self execScript:@"scanner.sh" withArgs:args];
}

-(void)onEnable:(id)sender
{    
    if([enable state] == NSOffState){
        // if we can't kill playdar don't pretend we did, unless the problem is
        // that our pid is invalid
        // FIXME I'm not so sure if KILL is safe... what's CTRL-C do?
        if(kill(pid, SIGKILL) == -1 && errno != ESRCH) {
            [enable setState:NSOnState];
            //TODO beep, show message
            return;
        }
        pid = 0;
    }else{
        pid = playdar_pid(); // for some reason assignment doesn't happen inside if statements..
        if(!pid){
            NSTask* task = [[NSTask alloc] init];
            @try{
                if([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask){
                    [task setLaunchPath:@"/usr/bin/open"];
                    [task setArguments:[NSArray arrayWithObjects:@"-a", @"Terminal", daemon_script_path(), nil]];
                    [task launch];
                    [task waitUntilExit];
                    pid = -100; //HACK FIXME
                }else{
                    [task setLaunchPath:daemon_script_path()];
                    [task launch];
                    pid = [task processIdentifier];
                }
            }
            @catch(NSException* e)
            {
                NSString* msg = @"The file at \"";
                msg = [msg stringByAppendingString:[task launchPath]];
                msg = [msg stringByAppendingString:@"\" could not be executed."];
                
                NSBeginAlertSheet(@"Could not start Playdar",
                                  nil, nil, nil,
                                  [[self mainView] window],
                                  self,
                                  nil, nil,
                                  nil,
                                  msg );
            }
            @finally {
                [task release];
            }
        }
    }
    
    bool const is_dead = pid == 0;
    
    // eg. if we're hidden, and playdar isn't running, then GUI representation is already correct
    if([info isHidden] != is_dead){    
        [demos setHidden:is_dead];
        [info setHidden:is_dead];  
        
        int const step = is_dead ? -20 : 20;
        NSWindow* w = [popup window];
        NSRect rect = [w frame];
        rect.size.height += step;
        rect.origin.y -= step;
        [w setFrame:rect display:true animate:true];
    }
    if([self isLoginItem] == is_dead)
        [self setLoginItem:!is_dead];
}

-(void)setLoginItem:(bool)enabled
{
	CFArrayRef loginItems = NULL;
	NSURL *url = [NSURL fileURLWithPath:daemon_script_path()];
	int existingLoginItemIndex = -1;
	OSStatus err = LIAECopyLoginItems(&loginItems);
	if(err == noErr) {
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
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)daemon_script_path(), kCFURLPOSIXPathStyle, false);
    OSStatus err = LIAECopyLoginItems(&loginItems);
    if(err == noErr) {
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
-(void)onSelect:(id)sender
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

-(NSTask*)execScript:(NSString*)script_name withArgs:(NSArray*)args
{
    NSTask* task = 0;
    @try {
        NSString* resources = [[self bundle] resourcePath];
        NSString* path = [resources stringByAppendingPathComponent:script_name];
        
        task = [[NSTask alloc] init];
        [task setCurrentDirectoryPath:resources];
        [task setLaunchPath:path];
        [task setArguments:args];
        [task launch];
    }
    @catch (NSException* e)
    {
        //TODO log - couldn't figure out easy way to do this
    }
    return task;
}

-(IBAction)onHelp:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://playdar.org"]];
}

-(IBAction)onAdvanced:(id)sender
{
    [NSApp beginSheet:advanced_window
       modalForWindow:[[self mainView] window]
        modalDelegate:self
       didEndSelector:nil
          contextInfo:nil];
}

-(IBAction)onCloseAdvanced:(id)sender
{
    if([[NSUserDefaults standardUserDefaults] boolForKey:@"Homemade"]){
        NSString* path = [self bin]; //use path that script eventually uses
        if([[NSFileManager defaultManager] isExecutableFileAtPath:path] == false){
            NSRunAlertPanel( @"Bad path", [path stringByAppendingString:@" isn't playdar"], nil, nil, nil );
            return;
        }
    }
    
    [advanced_window orderOut:nil];
    [NSApp endSheet:advanced_window];
    
    [self writeDaemonScript];
}

-(IBAction)onEditConfigFile:(id)sender;
{
    bool b = [[NSWorkspace sharedWorkspace] openFile:ini_path() withApplication:@"TextMate"];
    if (!b) [[NSWorkspace sharedWorkspace] openFile:ini_path() withApplication:@"TextEdit"];
}

-(NSString*)bin
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"Homemade"]
         ? [[[NSUserDefaults standardUserDefaults] stringForKey:@"HomemadePath"] stringByStandardizingPath]
         : [[[self bundle] bundlePath] stringByAppendingPathComponent:@"Contents/MacOS/playdar"];
}

-(void)writeDaemonScript
{
    NSString* path = daemon_script_path();
    NSString* cd = [[NSUserDefaults standardUserDefaults] boolForKey:@"Homemade"]
            ? @"cd `dirname $playdar`/..\n"
            : @"cd `dirname $playdar`\n";
    
    NSMutableString* command = [NSMutableString stringWithString:@"#!/bin/bash\n"];
    [command appendFormat:@"playdar='%@'\n", [self bin]];
    [command appendString:cd];
    [command appendFormat:@"exec $playdar -c '%@'\n", ini_path()];
    NSError* error;
    bool ok = [command writeToFile:path
                        atomically:true
                          encoding:NSUTF8StringEncoding
                             error:&error];
    
    if (!ok) {} //TODO
    
    NSDictionary* dict = [NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedInt:0755U]
                                                     forKey:NSFilePosixPermissions];
    
    [[NSFileManager defaultManager] changeFileAttributes:dict
                                                  atPath:path];
}

-(IBAction)onDemos:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://playdar.org/demos/"]];
}


-(IBAction)onViewStatus:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://localhost:8888"]];
}

@end


@implementation OrgPlaydarPreferencePane(SUUpdaterDelegateInformalProtocol)

-(void)updaterWillRelaunchApplication:(SUUpdater*)updater
{
    if(pid) kill(pid, SIGKILL);
}

-(NSString*)pathToRelaunchForUpdater:(SUUpdater*)updater
{
    return [[NSBundle mainBundle] bundlePath];
}

@end
