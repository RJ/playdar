/***************************************************************************
 *   Copyright 2008-2009 Last.fm Ltd.                                      *
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

#include "iPodDetector.h"
#include "Ipod.h"

#include "common/logger.h"
#include "Moose.h"

#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOPartitionScheme.h>
#include <IOKit/IOBSD.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>

#include <mach/mach.h>
#include <mach/error.h>

#include <sys/mount.h>

#include <fstream>

using namespace std;

static const double kTimeOutInterval = 5.0 * 60.0;  // 5 minute timeout before sync is forced


/** @author Jono Cole <jono@last.fm> 
  * @brief Used in the callback when the sync timer timesout
  */  
struct SyncTimeoutInfo
{
    IPodDetector* iPodDetector;
    std::string ipodSerial;
};


IPodDetector::IPodDetector()
             :m_detectorStarted( false ),
              m_threadError( false )
              
{
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_create( &m_threadId, &attr, threadEntry, this );
}


IPodDetector::~IPodDetector()
{
    if( !m_threadError )
    {
        CFRunLoopSourceSignal( m_quitSource );
        CFRunLoopWakeUp( CFRunLoopGetCurrent() );
    }
}


void* //static
IPodDetector::threadEntry( void* param )
{
    IPodDetector* ipd = static_cast<IPodDetector*>(param);
    
    /* setup native events to give mount / unmount detection
     * IOKit does not give this information as it is not at
     * the Hardware level.
     */
     
    EventHandlerUPP handlerUPP;
    handlerUPP = NewEventHandlerUPP( onMountStateChanged );
    EventTypeSpec eventTypes[] =
    {
        { kEventClassVolume, kEventVolumeMounted },
        { kEventClassVolume, kEventVolumeUnmounted }
    };

    OSStatus err = InstallApplicationEventHandler( handlerUPP, 2, eventTypes, param, NULL );
    //TODO error handling

     
     
    /* Set up IOKit to give notifications for hardware events
     * (USB and Firewire Device plugin / remove )
     * This is mainly needed to support non-mountable devices
     * such as the iPhone and iPod Touch. But is also used to
     * retrieve the USB serial number for all devices. 
     */
    
    if( !ipd->setupDetection() )
    {
        LOG( 2, "Error - could not initialize iPod / iPhone detection.\n"
                 "Devices will not be detected and wont scrobble." );
        ipd->m_threadError = true;
        return 0;
    }
    
    CFRunLoopSourceContext context = {
       0,           //version;
       ipd,         //void *info;
       NULL,        //CFAllocatorRetainCallBack retain;
       NULL,        //CFAllocatorReleaseCallBack release;
       NULL,        //CFAllocatorCopyDescriptionCallBack copyDescription;
       NULL,        //CFRunLoopEqualCallBack equal;
       NULL,        //CFRunLoopHashCallBack hash;
       NULL,        //CFRunLoopScheduleCallBack schedule;
       NULL,        //CFRunLoopCancelCallBack cancel;
       onQuit       //CFRunLoopPerformCallBack perform;
    };
    ipd->m_quitSource = CFRunLoopSourceCreate( kCFAllocatorDefault, 0, &context );
    CFRunLoopAddSource( CFRunLoopGetCurrent(), ipd->m_quitSource, kCFRunLoopDefaultMode );
    
    ipd->m_detectorStarted = true;
    CFRunLoopRun();
    
    LOG( 3, "Clearing up iPodMap" );
    //clear up memory
    map< std::string, IPod* >::iterator i;
    for( i = ipd->m_ipodMap.begin(); i != ipd->m_ipodMap.end(); ipd++ )
    {
        delete i->second;
    }
    ipd->m_ipodMap.clear();
    
    return 0;
}


bool
IPodDetector::setupDetection()
{
    kern_return_t ioResult;
    
    IONotificationPortRef notificationObject = IONotificationPortCreate( kIOMasterPortDefault );
    
    CFRunLoopSourceRef notificationRunLoopSource;

    //Use the notification object received from IONotificationPortCreate
    notificationRunLoopSource = IONotificationPortGetRunLoopSource(notificationObject);
                
    //Add the notification object to the event loop
    CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationRunLoopSource, kCFRunLoopDefaultMode);
    
    //Dictionary for matching iPhone / iPod connections
    CFMutableDictionaryRef iPhoneConnectMatch = IOServiceNameMatching( "iPhone" ); 
    CFMutableDictionaryRef iPodConnectMatch = IOServiceNameMatching( "iPod" ); 
    CFMutableDictionaryRef iPodMiniConnectMatch = IOServiceNameMatching( "iPod mini" ); 
        
    //Dictionaries for matching iPhone / iPod disconnections
    CFMutableDictionaryRef iPhoneDisconnectMatch = IOServiceNameMatching( "iPhone" ); 
    CFMutableDictionaryRef iPodDisconnectMatch = IOServiceNameMatching( "iPod" ); 
    CFMutableDictionaryRef iPodMiniDisconnectMatch = IOServiceNameMatching( "iPod mini" );
    
    //Dictionaries for matching Firewire devices
    CFMutableDictionaryRef fireWireConnectMatch = IOServiceMatching( "IOFireWireSBP2LUN" );
    CFMutableDictionaryRef fireWireDisconnectMatch = IOServiceMatching( "IOFireWireSBP2LUN" );
    
    //Dictionaries for mount and unmount
    CFMutableDictionaryRef deviceNodeAddedMatch = IOServiceMatching( "IOMedia" );
    CFMutableDictionaryRef deviceNodeRemovedMatch = IOServiceMatching( "IOMedia" );
    
    //These iterators will be used to check the current state of already
    //plugged in devices before the notifications start coming in.
    io_iterator_t iPhoneAddedIter;
    io_iterator_t iPhoneRemovedIter;
    io_iterator_t iPodAddedIter;
    io_iterator_t iPodRemovedIter;
    io_iterator_t iPodMiniAddedIter;
    io_iterator_t iPodMiniRemovedIter;
    io_iterator_t fireWireAddedIter;
    io_iterator_t fireWireRemovedIter;
    io_iterator_t deviceNodeAddedIter;
    io_iterator_t deviceNodeRemovedIter;
            
    //Add the notifications for connected / disconnected
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOMatchedNotification,
                                                 iPhoneConnectMatch, 
                                                 onIPhoneDetected, 
                                                 this, 
                                                 &iPhoneAddedIter );

    
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add iPhone connected notification: " << mach_error_string( ioResult ) );
        return false;
    }

    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOMatchedNotification,
                                                 iPodConnectMatch, 
                                                 onUsbIPodDetected, 
                                                 this, 
                                                 &iPodAddedIter );

    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add iPod connected notification: " << mach_error_string( ioResult ) );
        return false;
    }

    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOMatchedNotification,
                                                 iPodMiniConnectMatch, 
                                                 onUsbIPodDetected, 
                                                 this, 
                                                 &iPodMiniAddedIter );

    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add iPod Mini connected notification: " << mach_error_string( ioResult ) );
        return false;
    }
                                          
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOMatchedNotification,
                                                 fireWireConnectMatch, 
                                                 onFireWireDetected, 
                                                 this, 
                                                 &fireWireAddedIter );
                                                 
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add firewire connected notification: " << mach_error_string( ioResult ) );
        return false;
    }
    
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOTerminatedNotification,
                                                 iPhoneDisconnectMatch, 
                                                 onDeviceRemoved, 
                                                 this, 
                                                 &iPhoneRemovedIter );
                                                 
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add iPhone disconnected notification: " << mach_error_string( ioResult ) );
        return false;
    }
                                      
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOTerminatedNotification,
                                                 iPodDisconnectMatch, 
                                                 onDeviceRemoved, 
                                                 this, 
                                                 &iPodRemovedIter );
                                                 
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add iPod disconnected notification: " << mach_error_string( ioResult ) );
        return false;
    }
                                      
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOTerminatedNotification,
                                                 iPodMiniDisconnectMatch, 
                                                 onDeviceRemoved, 
                                                 this, 
                                                 &iPodMiniRemovedIter );
                                                 
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add iPod Mini disconnected notification: " << mach_error_string( ioResult ) );
        return false;
    }
                                       
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOTerminatedNotification,
                                                 fireWireDisconnectMatch, 
                                                 onFireWireRemoved, 
                                                 this, 
                                                 &fireWireRemovedIter );
                                                 
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add iPod Mini disconnected notification: " << mach_error_string( ioResult ) );
        return false;
    }
                                           
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOMatchedNotification,
                                                 deviceNodeAddedMatch, 
                                                 onDeviceNodeAdded, 
                                                 this, 
                                                 &deviceNodeAddedIter );
                                                 
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add device mount notification: " << mach_error_string( ioResult ) );
        return false;
    }        
                                       
    ioResult = IOServiceAddMatchingNotification( notificationObject, 
                                                 kIOTerminatedNotification,
                                                 deviceNodeRemovedMatch, 
                                                 onDeviceNodeRemoved, 
                                                 this, 
                                                 &deviceNodeRemovedIter );
                                                 
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not add device unmount notification: " << mach_error_string( ioResult ) );
        return false;
    }

    // Check if there are any devices already plugged in / disconnected
    // and 'arm' the notification mechanism
    onIPhoneDetected( this, iPhoneAddedIter );
    onUsbIPodDetected( this, iPodAddedIter );
    onUsbIPodDetected( this, iPodMiniAddedIter );
    onFireWireDetected( this, fireWireAddedIter );
    onDeviceRemoved( this, iPhoneRemovedIter );
    onDeviceRemoved( this, iPodRemovedIter );
    onDeviceRemoved( this, iPodMiniRemovedIter );
    onFireWireRemoved( this, fireWireRemovedIter );
    onDeviceNodeAdded( this, deviceNodeAddedIter );
    onDeviceNodeRemoved( this, deviceNodeRemovedIter );
    
    return true;
}


void //static
IPodDetector::onUsbIPodDetected( void* param, io_iterator_t newIterator )
{
    IPodDetector* ipd = static_cast<IPodDetector*>( param );
    io_object_t ioUsbDeviceNub;
    
    while( ioUsbDeviceNub = IOIteratorNext( newIterator ) )
    {
        IPod* iPod = IPod::newFromUsbDevice( ioUsbDeviceNub, false );
        if( iPod )
        {
            ipd->notifyIfUnknownIPod( iPod );

            if( iPod->isMobileScrobblerInstalled() )
            {
                LOG( 3, "Mobile Scrobbler detected on iPod Touch - client scrobbling not needed." );
                delete iPod;
                IOObjectRelease( ioUsbDeviceNub );
                continue;
            }
            
            ipd->m_ipodMap[ iPod->serial() ] = iPod;
            
            if( iPod->isManualMode() )
            {
                LOG( 3, "iPod detected in manual mode." );
                ipd->startTwiddlyWithIpodSerial( iPod->serial() );
            }
            else
            {
                //Either the iPod is in automatic mode OR it's an old iPod
                LOG( 3, "iPod detected" );
                ipd->startSyncTimer( iPod );    
            }
        }
        
        IOObjectRelease( ioUsbDeviceNub );
    }
}


void //static
IPodDetector::onIPhoneDetected( void* param, io_iterator_t newIterator )
{
    IPodDetector* ipd = static_cast<IPodDetector*>( param );
    io_object_t ioUsbDeviceNub;
    
    while( ioUsbDeviceNub = IOIteratorNext( newIterator ) )
    {
        IPod* iPod = IPod::newFromUsbDevice( ioUsbDeviceNub, true );
        
        if( iPod )
        {
            ipd->notifyIfUnknownIPod( iPod );

            if( iPod->isMobileScrobblerInstalled() )
            {  
                LOG( 3, "Mobile Scrobbler detected on iPhone - client scrobbling not needed." );
                delete iPod;
                IOObjectRelease( ioUsbDeviceNub );
                continue;
            }

            ipd->m_ipodMap[ iPod->serial() ] = iPod;
            
            if( iPod->isManualMode() )
            {
                LOG( 3, "iPhone detected in manual mode!" );
                ipd->startTwiddlyWithIpodSerial( iPod->serial(), "manualMode" );
            }
            else
            {
                LOG( 3, "iPhone detected in automatic mode!" );
                ipd->startSyncTimer( iPod );  
            }
        }
        IOObjectRelease( ioUsbDeviceNub );
    }
}


void //static
IPodDetector::onDeviceRemoved( void* param, io_iterator_t newIterator )
{
    IPodDetector* ipd = static_cast<IPodDetector*>( param );
    io_object_t deviceInfo;
    while( deviceInfo = IOIteratorNext( newIterator ) )
    {
        std::string serial;
        IPod::getUsbSerial( deviceInfo, &serial );

        ipd->startTwiddlyWithIpodSerial( serial, "unplug" );

        IOObjectRelease( deviceInfo );
    }
}


void //static
IPodDetector::onFireWireDetected( void* param, io_iterator_t newIterator )
{
    IPodDetector* ipd = static_cast<IPodDetector*>( param );
    io_object_t ioFireWireDeviceNub;
    kern_return_t ioResult;
    while( ioFireWireDeviceNub = IOIteratorNext( newIterator ) )
    {
        CFMutableDictionaryRef propertyDictionary;
        ioResult = IORegistryEntryCreateCFProperties( (io_registry_entry_t)ioFireWireDeviceNub, 
                                               &propertyDictionary, 
                                               kCFAllocatorDefault, 
                                               0 );

        if( ioResult != kIOReturnSuccess )
        {
            LOG( 3, "Could not get firewire device properties: " << mach_error_string( ioResult ) );
            IOObjectRelease( ioFireWireDeviceNub );
            continue;
        }
        
        CFStringRef cfProductName;
        cfProductName = (CFStringRef)CFDictionaryGetValue( propertyDictionary, CFSTR( "FireWire Product Name" ) );
        
        if( CFStringCompare( cfProductName, CFSTR( "iPod" ), 0 ) == kCFCompareEqualTo )
        {
            //This firewire device is an iPod!
            
            IPod* iPod = IPod::newFromFireWireDevice( ioFireWireDeviceNub );
            
            if( iPod )
            {
                ipd->notifyIfUnknownIPod( iPod );

                LOG( 3, "FireWire iPod plugged in" );
                ipd->m_ipodMap[ iPod->serial() ] = iPod;
                ipd->startSyncTimer( iPod );
            }
        }
        
        IOObjectRelease( ioFireWireDeviceNub );
    }
}


void //static
IPodDetector::onFireWireRemoved( void* param, io_iterator_t newIterator )
{
    IPodDetector* ipd = static_cast< IPodDetector* >( param );
    io_object_t ioFireWireDeviceNub;
    while( ioFireWireDeviceNub = IOIteratorNext( newIterator ) )
    {
        std::string serial;
        IPod::getFireWireSerial( ioFireWireDeviceNub, &serial );

        ipd->startTwiddlyWithIpodSerial( serial, "unplug" );

        IOObjectRelease( ioFireWireDeviceNub );
    }
}


void //static 
IPodDetector::onDeviceNodeAdded( void* param, io_iterator_t newIterator )
{
    IPodDetector* ipd = static_cast< IPodDetector* >( param );
    io_object_t device;
    
    while( device = IOIteratorNext( newIterator ) )
    {
        CFBooleanRef isWhole = (CFBooleanRef)IORegistryEntryCreateCFProperty( device,
                                                                              CFSTR( kIOMediaWholeKey ),
                                                                              kCFAllocatorDefault,
                                                                              0 );
        
        if( isWhole )
        {
            if( CFBooleanGetValue( isWhole ) )
            {
                IOObjectRelease( device );
                continue;
            } 
        }

        /* //TODO: try and use the following to determine the device name
        CFStringRef cfBsdName = (CFStringRef)IORegistryEntryCreateCFProperty( device,
                                                                              CFSTR( kIOBSDNameKey ),
                                                                              kCFAllocatorDefault,
                                                                              0 ); */
                                                                              
        CFNumberRef cfBsdUnit = (CFNumberRef)IORegistryEntryCreateCFProperty( device,
                                                                              CFSTR( kIOBSDUnitKey ),
                                                                              kCFAllocatorDefault,
                                                                              0 );
                                                                              
        CFNumberRef cfPartId  = (CFNumberRef)IORegistryEntryCreateCFProperty( device,
                                                                              CFSTR( kIOMediaPartitionIDKey ),
                                                                              kCFAllocatorDefault,
                                                                              0 );
        
        int bsdUnit, partitionId;
        CFNumberGetValue( cfBsdUnit, kCFNumberIntType, &bsdUnit );
        CFNumberGetValue( cfPartId, kCFNumberIntType, &partitionId );
        
        CFRelease( cfBsdUnit );
        CFRelease( cfPartId );
        
        std::stringstream deviceDev;
        deviceDev << "disk" << bsdUnit << "s" << partitionId;

        std::string serialNo;
        if( !IPod::getUsbSerial( device, &serialNo ) )
        {
            io_name_t className;
            IOObjectGetClass( device, className );
            LOG( 3, "Could not get serial - className: " << className );
        }

        if( ipd->m_ipodMap.find( serialNo ) == ipd->m_ipodMap.end() )
        {
            LOG( 3, "Error: something's gone wrong - cannot find iPod in the map!" );
            IOObjectRelease( device );
            continue;
        }
        
        IPod* ipod = ipd->m_ipodMap[ serialNo ];
                    
        if( ipd->m_detectorStarted )
        {
            /* Store the deviceID in the iPod object so that we can 
             * match it up with a mount point when it is mounted.
             */
             ipod->setDiskID( deviceDev.str() );
        }
        else
        {
            /* The media can only be guarunteed to be mounted by the operating system
             * if the device was connected before iTunes started.
             * (ie the detector has NOT started yet.)
             */
            std::string mountPoint = ipd->getMountPoint( deviceDev.str() );
            ipod->setMountPoint( mountPoint );
            ipod->populateIPodManual();
            if( ipod->isManualMode() )
            {
                ipd->startTwiddlyWithIpodSerial( ipod->serial(), "manual" );
            }
        }
        IOObjectRelease( device );
    }
}


std::string 
IPodDetector::getMountPoint( const std::string& deviceName ) const
{
    LOG( 3, deviceName << " scanned." );
    std::string devPath = "/dev/";
    devPath += deviceName;
 
    int mountCount = getfsstat( NULL, 0, MNT_WAIT );
    
    struct statfs * mountInfo = new struct statfs[ mountCount ];
    
    sleep( 1 );
    
    int result = getfsstat( mountInfo, sizeof( struct statfs ) * mountCount, MNT_WAIT );
    if ( result < 0 )
    {
        LOG( 3, "getfsstat error: " << result );
        return "";
    }
    
    std::string mountPoint = "";
    
    for( int i = 0; i < mountCount; i++ )
    {
        if( strncmp( devPath.c_str(), mountInfo[i].f_mntfromname, devPath.length() ) == 0 )
        {
            mountPoint = mountInfo[i].f_mntonname;
            break;
        }
    }
    
    delete[] mountInfo;
    return mountPoint;
}


void //static 
IPodDetector::onDeviceNodeRemoved( void* param, io_iterator_t newIterator )
{
    IPodDetector* ipd = static_cast< IPodDetector* >( param );
    io_object_t device;
    while( device = IOIteratorNext( newIterator ) )
    {
        kern_return_t ioResult;

        CFBooleanRef isLeaf = (CFBooleanRef)IORegistryEntryCreateCFProperty( device, 
                                                                             CFSTR( kIOMediaLeafKey ), 
                                                                             kCFAllocatorDefault, 
                                                                             0 );

        if( isLeaf )
        {
            if( CFBooleanGetValue( isLeaf ) )
            {
                LOG( 3, "Leaf node removal ignored" );
                IOObjectRelease( device );
                continue;
            }
        }
        
        io_iterator_t iter;
        ioResult = IORegistryEntryCreateIterator( device, 
                                                  kIOServicePlane, 
                                                  kIORegistryIterateParents | kIORegistryIterateRecursively, 
                                                  &iter);
        if( ioResult != kIOReturnSuccess )
        {
            LOG( 3, "Could not create iterator to iterate over unmounted device" << mach_error_string( ioResult ) );
        }
        
        LOG( 3, "determining ipod unmount details" );
        io_object_t curParent;
        while( curParent = IOIteratorNext( iter ) )
        {
            io_name_t deviceName;
            ioResult = IORegistryEntryGetName( curParent, deviceName );
            if( ioResult != kIOReturnSuccess )
            {
                LOG( 3, "Could not get unmount parent's name/class: " << mach_error_string( ioResult ) );
                IOObjectRelease( curParent );
                continue;
            }
            
            if( strncmp( deviceName, "iPod", 4 ) == 0 ||
                strncmp( deviceName, "iPod mini", 9 ) == 0 )
            {
                LOG( 3, "iPod has been unmounted" );
                std::string serial;
                if( !IPod::getUsbSerial( curParent, &serial ) )
                {
                    LOG( 3, "Error: could not determine serial number of iPod from unmount event" );
                    IOObjectRelease( curParent );
                    continue;
                }
                ipd->startTwiddlyWithIpodSerial( serial, "unmount" );
            }
            if( strncmp( deviceName, "IOFireWireSBP2LUN", 17 ) == 0 )
            {
                LOG( 3, "iPod has been unmounted" );
                std::string serial;
                if( !IPod::getFireWireSerial( curParent, &serial ) )
                {
                    LOG( 3, "Error: could not determine serial number of iPod from unmount event" );
                    IOObjectRelease( curParent );
                    continue;
                }
                ipd->startTwiddlyWithIpodSerial( serial, "unmount" );
            }
            IOObjectRelease( curParent );
        }
            
        IOObjectRelease( device );
    }
}


OSStatus //Static
IPodDetector::onMountStateChanged(  EventHandlerCallRef handlerCallRef,
                                    EventRef event,
                                    void* userData )
{
    IPodDetector* ipd = static_cast<IPodDetector*>( userData );
    unsigned int eventKind = GetEventKind( event );
    unsigned int eventClass = GetEventClass( event );
    
    switch( eventClass )
    {
        case kEventClassVolume:
        {
            switch( eventKind )
            {
                case kEventVolumeMounted:
                {
                    FSVolumeRefNum volNum = 0;
                    OSStatus result = GetEventParameter( event, kEventParamDirectObject, typeFSVolumeRefNum,
                                       NULL, sizeof( volNum ), NULL, &volNum );
                    if( result != noErr )
                    {
                        //This has been known to be caused by iPod firmware update / restore
                        LOG( 3, "Could not get event parameters. This mount will be ignored" );
                        break;
                    }
                    
                    CFStringRef cfDiskID = NULL;
                    result = FSCopyDiskIDForVolume( volNum, &cfDiskID );
                    
                    if( result != noErr )
                    {
                        LOG( 3, "Could not get diskID from mounted volume. This mount will be ignored" );
                        break;
                    }
                    
                    char diskID[255];
                    CFStringGetCString( cfDiskID, diskID, sizeof( diskID ), kCFStringEncodingASCII );
                    CFRelease( cfDiskID );
                    
                    LOG( 3, "mounted Volume location = /dev/" << diskID );

                
                    std::string volumePath = ipd->getMountPoint( diskID );
                    
                    LOG( 3, "Volume mounted at location: " << volumePath );
                    
                    std::map< std::string, IPod* >::iterator mapIter;
                    for( mapIter = ipd->m_ipodMap.begin(); 
                         mapIter != ipd->m_ipodMap.end(); )
                    {
                        IPod* curIpod = mapIter->second;
                        const std::string& curDiskID = curIpod->diskID();
                        
                        mapIter++;
                        
                        if( strncmp( curDiskID.c_str(), diskID, curDiskID.length() ) != 0 )
                            continue;

                        curIpod->setMountPoint( volumePath );
                        curIpod->populateIPodManual();

                        if( curIpod->isManualMode() )
                        {
                            LOG( 3, "iPod detected in manual sync mode." );
                            ipd->startTwiddlyWithIpodSerial( curIpod->serial(), "manual" );
                        }
                        else
                        {
                            LOG( 3, "iPod detected in automatic sync mode." );
                        }
                        
                    }
                }
                break;
                case kEventVolumeUnmounted:
                {
                    LOG( 3, "kEventVolumeUnmounted event received." );
                }
                break;
            }
        }
        break;
        default:
        assert( !"Unexpected event received from the carbon event system." );
    }
    return noErr;
}


void 
IPodDetector::startSyncTimer( IPod* const ipod )
{
    CFRunLoopTimerRef timer;
    
    SyncTimeoutInfo* info = new SyncTimeoutInfo();
    info->iPodDetector = this;
    info->ipodSerial = ipod->serial();
    
    CFRunLoopTimerContext context = {
                0,                              //version (must be 0)
                info,                           //parameter to pass to callback
                NULL,                           //retain callback
                onSyncTimerRelease,             //release callback
                NULL                            //copy description callback
    };
    
    timer = CFRunLoopTimerCreate (
                kCFAllocatorDefault,                              //Allocator
                CFAbsoluteTimeGetCurrent () + kTimeOutInterval,   //fireDate
                0,                                                //interval 0 == one-shot timer
                0,                                                //flags (for future compatibility)
                0,                                                //order (currently ignored by runloops?!)
                onSyncTimerFire,                                  //callback method
                &context                                          //context information
    );
    CFRunLoopAddTimer( CFRunLoopGetCurrent(), timer, kCFRunLoopDefaultMode );
    CFRunLoopWakeUp( CFRunLoopGetCurrent() );   
}

void //static
IPodDetector::onSyncTimerFire(  CFRunLoopTimerRef timer, void *info )
{
    SyncTimeoutInfo* timeoutInfo = static_cast<SyncTimeoutInfo*>( info );
    timeoutInfo->iPodDetector->startTwiddlyWithIpodSerial( timeoutInfo->ipodSerial, "timeout" );
}


void //static
IPodDetector::onSyncTimerRelease( const void *info )
{
    const SyncTimeoutInfo* timeoutInfo = static_cast<const SyncTimeoutInfo*>( info );
    delete timeoutInfo;
}


void //static
IPodDetector::onQuit( void* param )
{
    IPodDetector* ipd = static_cast< IPodDetector* >( param );
    CFRunLoopStop( CFRunLoopGetCurrent() );
    pthread_join(  ipd->m_threadId, NULL );
}


void
IPodDetector::notifyIfUnknownIPod( IPod* ipod )
{
    // Qt unexpectedly separates with '.'
    std::string tmp = "device." + ipod->device() + '.' + ipod->serial() + ".user";
    CFStringRef key = CFStringCreateWithBytes( NULL,
                                              (const unsigned char*) tmp.c_str(),
                                               tmp.length(),
                                               kCFStringEncodingUTF8,
                                               false );
    if (!key)
        return;

    CFStringRef appId = CFSTR( "fm.last" );
    CFStringRef user = (CFStringRef) CFPreferencesCopyAppValue( key, appId );

    if (user == NULL)
    {
        CFPreferencesSetAppValue( key, CFSTR( "" ), appId );
        CFPreferencesAppSynchronize( appId );
        Moose::exec( Moose::applicationPath(), "--ipod-detected" );
    }
    else
        CFRelease( user );

    CFRelease( key );
}
