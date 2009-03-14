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


//TODO watch exit code of playdard (if early exits), and watch process while pref pane is open
// ^^ important in case playdard exits immediately due to crash or somesuch

//TODO remember path that we scanned with defaults controller
//TODO memory leaks
//TODO log that stupid exception
//TODO sparkle updates
// ^^ ensure if prefpane is updated by sparkle, it restarts playdar
//TODO while open auto restart playdar binary if using byo_bin and playdar binary is modified


#import "main.h"
#include <sys/sysctl.h>
#include <Sparkle/SUUpdater.h>
#include "LoginItemsAE.h"


/** returns the pid of the running playdard instance, or 0 if not found */
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


@implementation OrgPlaydarPreferencePane

-(void)mainViewDidLoad
{   
    NSString* ini = ini_path();
    if([[NSFileManager defaultManager] fileExistsAtPath:ini] == false) 
    {
        NSArray* args = [NSArray arrayWithObjects: fullname(), db_path(), ini, nil];
        [self execScript:@"playdar.ini.rb" withArgs:args];
    }

    [[popup menu] addItem:[NSMenuItem separatorItem]];
    [[[popup menu] addItemWithTitle:@"Select..." action:@selector(onSelect:) keyEquivalent:@""] setTarget:self];

    NSString* home = NSHomeDirectory();
    [self addFolder:[home stringByAppendingPathComponent:@"Music"] setSelected:true];
    [self addFolder:home setSelected:false];
        
    if(pid = playdar_pid()) [self updateStatusTextFields];
    if(![self isLoginItem]) [check setState:NSOffState];

    [[SUUpdater updaterForBundle:[self bundle]] resetUpdateCycle];
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

-(void)onStart:(id)sender
{
    NSMutableArray* args = [NSArray arrayWithObjects:@"-c", ini_path(), nil];
    
    if(pid) {
        if(kill( pid, SIGKILL ) != 0 && playdar_pid()) return;
        pid = 0;
    }
    else if([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
    {
        [[self execScript:@"playdar.sh" withArgs:args] waitUntilExit];
        pid = playdar_pid();
    }
    else {
        NSTask *task = [[NSTask alloc] init];
        @try
        {
            [task setLaunchPath:[self daemon]];
            [task setArguments:args];
            [task launch];
            pid = [task processIdentifier];
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
    }
    [self updateStatusTextFields];
}

-(void)updateStatusTextFields
{
    if (pid) {
        [start setTitle:@"Stop Playdar"];
        [status setStringValue:@"Running"];
    } else {
        [start setTitle:@"Start Playdar"];
        [status setStringValue:@"Not running"];
    }
}

-(void)onStartAtLogin:(id)sender
{
    bool const enabled = [check state] == NSOnState;
	CFArrayRef loginItems = NULL;
	NSURL *url = [NSURL fileURLWithPath:[self daemon]];
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
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)[self daemon], kCFURLPOSIXPathStyle, false);
    NSLog( @"%@", url );
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
    [advanced_window orderOut:nil];
    [NSApp endSheet:advanced_window];
}

-(IBAction)onEditPlaydarIni:(id)sender;
{
    [[NSWorkspace sharedWorkspace] openFile:ini_path()];
}

-(NSString*)daemon
{
    NSUserDefaults* defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
    if ([defaults boolForKey:@"byoBinaries"]) {
        NSString* path = [defaults stringForKey:@"byoBinariesPath"];
        if (path && [path length])
            return [path stringByAppendingPathComponent:@"playdar"];
    }
    return [[[self bundle] bundlePath] stringByAppendingPathComponent:@"Contents/MacOS/playdar"];
}

@end
