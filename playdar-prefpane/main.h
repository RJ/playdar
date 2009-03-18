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

#import <PreferencePanes/PreferencePanes.h>

// long class name is because we get loaded into System Preferences process, so 
// we are required to be ultra verbose
// http://developer.apple.com/documentation/UserExperience/Conceptual/PreferencePanes/Tasks/Conflicts.html
@interface OrgPlaydarPreferencePane : NSPreferencePane 
{
    IBOutlet NSPopUpButton* popup;
    IBOutlet NSButton* scan;
    IBOutlet NSButton* enable;
    IBOutlet NSButton* demos;
    IBOutlet NSTextField* info;
    IBOutlet NSWindow* advanced_window;
    pid_t pid;
}

-(void)mainViewDidLoad;
-(void)addFolder:(NSString*)path setSelected:(bool)select;

-(NSTask*)execScript:(NSString*)command withArgs:(NSArray*)args;
-(NSString*)bin;
-(void)writeDaemonScript;

-(bool)isLoginItem;
-(void)setLoginItem:(bool)enabled;

-(void)openPanelDidEnd:(NSOpenPanel*)panel
            returnCode:(int)returnCode
           contextInfo:(void*)contextInfo;



-(IBAction)onSelect:(id)sender;
-(IBAction)onScan:(id)sender;
-(IBAction)onEnable:(id)sender;
-(IBAction)onDemos:(id)sender;
-(IBAction)onHelp:(id)sender;
-(IBAction)onAdvanced:(id)sender;
-(IBAction)onEditConfigFile:(id)sender;
-(IBAction)onViewStatus:(id)sender;
-(IBAction)onCloseAdvanced:(id)sender;

@end
