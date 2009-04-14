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

#include "IPod.h"
#include <dirent.h>
#include <sys/stat.h>
#include <mach/mach.h>
#include <mach/error.h>
#include <IOKit/usb/IOUSBLib.h>


IPod* //static
IPod::newFromUsbDevice( io_object_t device, bool isIPhone /* = false */ )
{
    IPod* ipod = new IPod();

    io_object_t usbDevice = device;
    io_name_t className;
    IOObjectGetClass( usbDevice, className );
    if( strncmp( className, kIOUSBDeviceClassName, sizeof( kIOUSBDeviceClassName ) ) != 0 )
    {
        usbDevice = getBaseDevice( device );
        if( usbDevice == NULL )
        {
            LOG( 3, "Error could not find base USBDevice for device class: " << className );
            return NULL;
        }
    }
    
    ipod->m_connectionType = usb;
    if ( !getUsbSerial( device, &ipod->m_serial ) )
    {
        delete ipod;
        return NULL;
    }
    
    try
    {
        if( isIPhone )
        {
            //IPhone
            ipod->m_type = iPhone;
        }
        else if( ipod->isOldIpod( device ) )
        {
            //IPod classic
            ipod->m_type = iPod;
        }
        else 
        {
            //IPod Touch
            ipod->m_type = iTouch;
        }
    }
    catch ( const kern_return_t e )
    {
        // the kern_return_t error number is thrown if isOldIpod fails
        // (the success can't be determined by the return value)
        delete ipod;
        return NULL;
    }
    
    if( ipod->m_type == iTouch ||
        ipod->m_type == iPhone )
    {
        if( ipod->waitForIpodBackup() )
        {
            ipod->m_mobileScrobblerInstalled = ipod->queryMobileScrobblerInstalled();
            ipod->m_manualMode = ipod->queryIPhoneManual();
        }
        else
        {
            LOG( 3, "Warning: could not determine information from iPhone / iPod touch backup information\n"
                     "The presumption is that this device is in automatic sync mode without mobile scrobbler installed" );
        }
    }
    
    if( !ipod->getDeviceId( device, &ipod->m_vid, &ipod->m_pid ) )
    {
        delete ipod;
        return NULL;
    }
    
    if( usbDevice != device )
    {
        IOObjectRelease( usbDevice );
    }
    return ipod;
}


IPod* //static
IPod::newFromFireWireDevice( io_object_t device )
{
    IPod* ipod = new IPod();
    ipod->m_connectionType = fireWire;
    if ( !getFireWireSerial( device, &ipod->m_serial ) )
    {
        delete ipod;
        return NULL;
    }

    //We can presume that all firewire IPods are classic iPods
    //( Apple deprecated firewire connections with iPods after 4G )
    ipod->m_type = iPod;
    
    if( !ipod->getDeviceFireWireId( device, &ipod->m_vid, &ipod->m_pid ) )
    {
        delete ipod;
        return NULL;
    }
    return ipod;
}


bool 
IPod::getDeviceId( io_object_t device, int* vid, int* pid )
{
    CFMutableDictionaryRef propertyDictionary;
    kern_return_t ioResult;
    ioResult = IORegistryEntryCreateCFProperties( (io_registry_entry_t)device, 
                                                   &propertyDictionary, 
                                                   kCFAllocatorDefault, 
                                                   0 );

    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not get usb device properties: " << mach_error_string( ioResult ) );
        return false;
    }

    CFNumberRef cfVendor;
    cfVendor = (CFNumberRef)CFDictionaryGetValue( propertyDictionary, CFSTR( "idVendor" ) );

    CFNumberRef cfProduct;    
    cfProduct = (CFNumberRef)CFDictionaryGetValue( propertyDictionary, CFSTR( "idProduct" ) );
    
    CFNumberGetValue ( cfVendor, kCFNumberIntType, vid );
    CFNumberGetValue ( cfProduct, kCFNumberIntType, pid );

    CFRelease( cfVendor );
    CFRelease( cfProduct );
    return true;
}


bool 
IPod::getDeviceFireWireId( io_object_t device, int* vid, int* pid )
{
    CFMutableDictionaryRef propertyDictionary;
    kern_return_t ioResult;
    ioResult = IORegistryEntryCreateCFProperties( (io_registry_entry_t)device, 
                                                   &propertyDictionary, 
                                                   kCFAllocatorDefault, 
                                                   0 );

    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not get firewire device properties: " << mach_error_string( ioResult ) );
        return false;
    }

    CFNumberRef cfVendor;
    cfVendor = (CFNumberRef)CFDictionaryGetValue( propertyDictionary, CFSTR( "Vendor_ID" ) );

    CFNumberRef cfProduct;    
    cfProduct = (CFNumberRef)CFDictionaryGetValue( propertyDictionary, CFSTR( "Model_ID" ) );
    
    CFNumberGetValue ( cfVendor, kCFNumberIntType, vid );
    CFNumberGetValue ( cfProduct, kCFNumberIntType, pid );

    CFRelease( cfVendor );
    CFRelease( cfProduct );
    return true;
}


bool //static
IPod::getUsbSerial( io_object_t device, std::string* serialOut )
{

    io_object_t usbDevice = device;
    io_name_t className;
    IOObjectGetClass( usbDevice, className );
    if( strncmp( className, kIOUSBDeviceClassName, sizeof( kIOUSBDeviceClassName ) ) != 0 )
    {
        usbDevice = getBaseDevice( device );
        if( usbDevice == NULL )
        {
            LOG( 3, "Error could not find base USBDevice for device class: " << className );
            return NULL;
        }
    }

    CFMutableDictionaryRef propertyDictionary;
    kern_return_t ioResult;
    ioResult = IORegistryEntryCreateCFProperties( (io_registry_entry_t)usbDevice, 
                                                   &propertyDictionary, 
                                                   kCFAllocatorDefault, 
                                                   0 );

    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not get serial properties: " << mach_error_string( ioResult ) );
        return false;
    }
    
    CFStringRef cfSerial;
    cfSerial = (CFStringRef)CFDictionaryGetValue( propertyDictionary, CFSTR( "USB Serial Number" ) );
    if( cfSerial == NULL )
    {
        LOG( 3, "Error - Could not get USB Serial Number" );
        return false;
    }
    
    char serial[50];
    CFStringGetCString( cfSerial, serial, 50, kCFStringEncodingASCII );

    CFRelease( cfSerial );
    *serialOut = serial;
    
    if( usbDevice != device )
    {
        IOObjectRelease( usbDevice );
    }
    return true;
}


bool //static
IPod::getFireWireSerial( io_object_t device, std::string* serialOut )
{
    CFMutableDictionaryRef propertyDictionary;
    kern_return_t ioResult;
    ioResult = IORegistryEntryCreateCFProperties( (io_registry_entry_t)device, 
                                                   &propertyDictionary, 
                                                   kCFAllocatorDefault, 
                                                   0 );

    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not get USB device properties: " << mach_error_string( ioResult ) );
        return false;
    }
    
    CFNumberRef cfSerial;
    cfSerial = (CFNumberRef)CFDictionaryGetValue( propertyDictionary, CFSTR( "GUID" ) );
    if( cfSerial == NULL )
    {
        LOG( 3, "GUID property does not exist for this device" );
        return false;
    } 

    int serial;

    CFNumberGetValue( cfSerial, kCFNumberIntType, &serial );
    
    
    std::stringstream serialStream;
    serialStream << "0x" << std::hex << serial;
    *serialOut = serialStream.str();

    CFRelease( cfSerial );
    
    return true;
}


bool 
IPod::isOldIpod( io_object_t device )
{
    io_iterator_t iter;
    kern_return_t ioResult;
    ioResult = IORegistryEntryCreateIterator( (io_registry_entry_t)device, 
                                               kIOServicePlane, 
                                               kIORegistryIterateRecursively, 
                                               &iter );

    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not create isOldIpod recursive iterator: " << mach_error_string( ioResult ) );
        throw ioResult;
    }
    
    io_object_t curObject;
    while( curObject = IOIteratorNext( iter ) )
    {
        io_name_t className;
        ioResult = IOObjectGetClass( curObject, className );
        
        if( ioResult != kIOReturnSuccess )
        {
            LOG( 3, "Could not get class name from current object: " << mach_error_string( ioResult ) );
            throw ioResult;
        }

        IOObjectRelease( curObject );
        if( strncmp( (char*)className, "IOUSBMassStorageClass", sizeof( className ) ) == 0 )
        {
            return true;
        }
    }
    return false;
}


CFDictionaryRef 
IPod::createDictionaryFromXML( CFStringRef filePath ) const
{
    CFDictionaryRef dictionary = NULL;
    CFDataRef resourceData = NULL;
    CFStringRef errorString = NULL;
    SInt32 errorCode;

    CFURLRef fileURL;
    fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault,
                                             filePath,                // file path name
                                             kCFURLPOSIXPathStyle,    // interpret as POSIX path
                                             false );                 // is it a directory?

    Boolean result = CFURLCreateDataAndPropertiesFromResource( kCFAllocatorDefault,      // allocator
                                                               fileURL,                  // file location
                                                               &resourceData,            // place to put file data
                                                               NULL,                     // properties
                                                               NULL,                     // properties which are desired
                                                               &errorCode);              // error code

    if( !result )
    {
        LOG( 3, "Error: could not get data from backup file:" << CFStringGetCStringPtr ( filePath, kCFStringEncodingASCII ) );
        CFRelease( fileURL );
        return dictionary;
    }

    dictionary = (CFDictionaryRef)CFPropertyListCreateFromXMLData( kCFAllocatorDefault,
                                                                       resourceData,
                                                                       kCFPropertyListImmutable,
                                                                       &errorString);
    CFRelease( resourceData );
    CFRelease( fileURL );
    return dictionary;
}


bool 
IPod::queryIPhoneManual() const
{

    CFDictionaryRef iTunesFiles;
    UInt8 manualMode;
    CFDataRef iTunesPrefsData;
    
    CFMutableStringRef infoPlistPath = CFStringCreateMutable( kCFAllocatorDefault, 0 );
    CFStringAppendCString( infoPlistPath, ::getenv( "HOME" ), kCFStringEncodingASCII );
    CFStringAppendCString( infoPlistPath, "/Library/Application Support/MobileSync/Backup/", kCFStringEncodingASCII );
    CFStringAppendCString( infoPlistPath, m_serial.c_str(), kCFStringEncodingASCII );
    CFStringAppendCString( infoPlistPath, "/Info.plist", kCFStringEncodingASCII );
    
    CFDictionaryRef propertyList = createDictionaryFromXML( infoPlistPath );
    CFRelease( infoPlistPath );
    
    if( propertyList == NULL )
    {
        LOG( 3, "Error: Could not read Info.plist file - presuming automatic sync enabled" );
        return false;
    }

    iTunesFiles = (CFDictionaryRef)CFDictionaryGetValue( propertyList, CFSTR( "iTunes Files" ) );
    
    if( iTunesFiles == NULL )
    {
        LOG( 3, "Error: Could not extract the iTunes Files property from Info.plist file. "
                 "Presuming automatic sync enabled." );
        return false;
    }
    
    iTunesPrefsData = (CFDataRef)CFDictionaryGetValue( iTunesFiles, CFSTR( "iTunesPrefs" ) );    
    
    if( iTunesPrefsData == NULL )
    {
        LOG( 3, "Error: Could not read the iTunesPrefs data from Info.plist - iTunesFiles section. "
                 "Presuming automatic sync enabled." );
        return false;
    }
    
    if( CFDataGetLength( iTunesPrefsData ) < 11 )
    {
        LOG( 3, "Error: Data length of iTunesPrefs data is too small. "
                 "Presuming automatic sync enabled." );
        return false;
    }
    
    /* The 10th byte of the iTunesPrefs file determines whether the iPod is
     * in manual or automatic mode.
     * Automatic == 1, Manual == 0
     */ 
    CFDataGetBytes( iTunesPrefsData, CFRangeMake( 10, 1 ), &manualMode );

    CFRelease( iTunesPrefsData );
    return (manualMode == 0);
}


void 
IPod::populateIPodManual()
{
    CFMutableStringRef iTunesPrefsPath = CFStringCreateMutable( kCFAllocatorDefault, 0 );
    CFStringAppendCString( iTunesPrefsPath, m_mountPoint.c_str(), kCFStringEncodingASCII );
    CFStringAppendCString( iTunesPrefsPath, "/iPod_Control/iTunes/iTunesPrefs", kCFStringEncodingASCII );
    
    
    CFURLRef fileURL;
    fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault,
                                             iTunesPrefsPath,         // file path name
                                             kCFURLPOSIXPathStyle,    // interpret as POSIX path
                                             false );                 // is it a directory?
                                             
    CFRelease( iTunesPrefsPath );
                                             
    CFDataRef iTunesPrefsData;
    
    SInt32 errorCode;
    Boolean result = CFURLCreateDataAndPropertiesFromResource( kCFAllocatorDefault,      // allocator
                                                               fileURL,                  // file location
                                                               &iTunesPrefsData,         // place to put file data
                                                               NULL,                     // properties
                                                               NULL,                     // properties which are desired
                                                               &errorCode);              // error code
                                                               
    if( !result ||
        CFDataGetLength( iTunesPrefsData ) < 11 )
    {
        LOG( 3, "Error: Data length of iTunesPrefs data is too small. "
                 "Presuming automatic sync enabled. " << std::endl <<
                 "MountPoint: " << m_mountPoint );
        m_manualMode = false;
        return;
    }
    
    
    UInt8 manualMode;
    /* The 10th byte of the iTunesPrefs file determines whether the iPod is
     * in manual or automatic mode.
     * Automatic == 1, Manual == 0
     */ 
    CFDataGetBytes( iTunesPrefsData, CFRangeMake( 10, 1 ), &manualMode );

    CFRelease( iTunesPrefsData );
    m_manualMode = (manualMode == 0);
}


io_object_t //static
IPod::getBaseDevice( io_object_t device )
{
    io_iterator_t parentIter;
    IOReturn ioResult = IORegistryEntryCreateIterator( device, 
                                              kIOServicePlane, 
                                              kIORegistryIterateParents | kIORegistryIterateRecursively, 
                                              &parentIter);
    if( ioResult != kIOReturnSuccess )
    {
        LOG( 3, "Could not create iterator to iterate over unmounted device" << mach_error_string( ioResult ) );
    }
    
    io_object_t curParent;
    while( curParent = IOIteratorNext( parentIter ) )
    {
        io_name_t deviceName;
        ioResult = IORegistryEntryGetName( curParent, deviceName );
        if( ioResult != kIOReturnSuccess )
        {
            LOG( 3, "Could not get parent's name/class: " << mach_error_string( ioResult ) );
            IOObjectRelease( curParent );
            continue;
        }
        
        if( strncmp( deviceName, "iPod", 4 ) == 0 ||
            strncmp( deviceName, "iPod mini", 9 ) == 0 ||
            strncmp( deviceName, "IOFireWireSBP2LUN", 17 ) == 0 )
        {
            return curParent;
        }
        IOObjectRelease( curParent );
    }
    return NULL;
}


bool 
IPod::queryMobileScrobblerInstalled() const
{
    DIR* plistFolder;
    struct dirent *dirEntry;
    
    std::string backupPath = ::getenv( "HOME" );
    backupPath += "/Library/Application Support/MobileSync/Backup/";
    backupPath += m_serial;
    backupPath += "/";
    
    plistFolder = opendir( backupPath.c_str() );
    if( plistFolder == NULL )
    {
        LOG( 3, "Could not query itouch / iphone backup folder - presuming MobileScrobbler is NOT installed.." );
        return false;
    }
    while( dirEntry = readdir( plistFolder ) )
    {
        if( dirEntry->d_type != DT_REG )
            continue;
        
        std::string curPath = backupPath + dirEntry->d_name;
        
        if( isMobileScrobblerPlist( curPath.c_str() ) )
            return true;
    }
    return false;
}

bool 
IPod::isMobileScrobblerPlist( const char* path ) const
{
    std::ifstream fin( path );
    fin.seekg( 0, std::ios_base::end );
    int length = fin.tellg();
    fin.seekg( 0, std::ios_base::beg );
    char* buffer = new char[ length ];
    fin.read( buffer, length );

    bool retval = false;
    if(strstr( buffer, "org.c99.MobileScrobbler.plist" ))
        retval = true;

    delete[] buffer;
    return retval;
}

bool 
IPod::waitForIpodBackup() const
{
    std::string backupPath = ::getenv( "HOME" );
    backupPath += "/Library/Application Support/MobileSync/Backup/";
    backupPath += m_serial;
    backupPath += "/Info.plist";

    struct stat infoPlistStat;
    long int curTime = time( NULL );
    long int infoPlistModTime = 0;
    int count;
    const int timeout = 60 * 5; //5 minute timeout
    for( count = 0; count < timeout && infoPlistModTime < curTime; count++ )
    {
        sleep( 1 ); //sleep 1 second
        if( stat( backupPath.c_str(), &infoPlistStat ) )
        {
            //TODO: error handling pls
        }
        infoPlistModTime = infoPlistStat.st_mtimespec.tv_sec;
    }
    return count != timeout;
}