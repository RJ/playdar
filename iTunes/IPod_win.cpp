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

#include <cstdio>
#include <string>
#include <ctime>

#include <comdef.h>
#include <Wbemidl.h>

#include <shlobj.h>
#include <sys/stat.h>
#include "Plist.h"

IPod* //static
IPod::newFromUsbDevice( IWbemClassObject *device, IWbemServices* wmiServices )
{
    IPod* ipod = new IPod;

    ipod->m_wmiServices = wmiServices;

    std::wstring name = ipod->getDeviceName( device );

    getSerialVidPid( device, &ipod->m_serial, &ipod->m_vid, &ipod->m_pid );

    if( L"Apple Mobile Device USB Driver" == name )
    {
        //iPhone / iPod Touch Detected
        ipod->m_type = iPhone;
        ipod->populateIPhoneMobileScrobblerAndManualMode();
    }
    else if( L"unknown" == name )
    {
        ipod->m_type = unknown;
    }
    else
    {
        ipod->m_type = iPod;
        ipod->populateMountPoint( device );
        if( ipod->m_mountPoint.length() < 1 )
        {
            LOGL( 3, "No mount point found - presuming automatic sync mode" );
        }
        else
        {
            ipod->populateIPodManual();
        }
    }

    if( L"Apple Computer_ Inc. iPod IEEE 1394 SBP2 Device" == name )
        ipod->m_connectionType = fireWire;
    else
        ipod->m_connectionType = usb;


    ipod->m_connectionTickCount = ::GetTickCount();

    return ipod;
}

void //static
IPod::getSerialVidPid( IWbemClassObject* const object, 
                       std::wstring *const serialOut /* = NULL */, 
                       int *const vidOut /* = NULL */, 
                       int *const pidOut /* = NULL */ )
{
    HRESULT result;
    VARIANT vtDeviceId;

    std::wstring deviceId;

    //get full pnpDevice Id String
    result = object->Get( L"DeviceID", 0, &vtDeviceId, 0, 0 );
    if( SUCCEEDED(result) )
    {
        deviceId = std::wstring( _bstr_t( vtDeviceId.bstrVal ) );
    }
    else
    {
        LOGL( 3, "Error getting DeviceID from WMI: 0x" << std::hex << result );
        VariantClear( &vtDeviceId );
        if( vidOut )
            *vidOut = 0;
        if( pidOut )
            *pidOut = 0;
        if( serialOut )
            *serialOut = L"unknown";
        return;
    }

    //seperate deviceId into tokens delimited by a '\'
    std::vector<std::wstring> tokens;
    tokenize( deviceId, tokens, L"\\" );

    if( tokens.size() < 3 )
    {
        LOGWL( 3, L"Could not determine serial / pid / vid from deviceID string: " << deviceId );
         VariantClear( &vtDeviceId );
         if( vidOut )
            *vidOut = 0;
         if( pidOut )
            *pidOut = 0;
         if( serialOut )
            *serialOut = L"unknown";
        return;
    }
    
    //serial number ( + optional &APPL0 if iPod classic ) is always 3rd element of vector
    std::vector<std::wstring> serialTokens;
    tokenize( tokens.at(2), serialTokens, L"&" );

    if( serialOut )
        *serialOut = serialTokens.at( 0 );

    // vid / pid information is in 2nd element of vector
    // split on &
    std::vector<std::wstring> vidPidTokens;
    tokenize( tokens.at(1), vidPidTokens, L"&" );
    
    if( vidPidTokens.size() < 2 )
    {
        LOGWL( 3, L"Could not determine  pid / vid from vidpid string: " << tokens.at(1) );
        VariantClear( &vtDeviceId );
        if( vidOut )
            *vidOut = 0;
        if( pidOut )
            *pidOut = 0;
        return;
    }

    std::wstring vid = vidPidTokens.at( 0 );
    std::wstring pid = vidPidTokens.at( 1 );

    std::wstringstream vidStream( vid.substr( 4, vid.length() - 4 ) );
    std::wstringstream pidStream( pid.substr( 4, pid.length() - 4 ) );

    //convert hex string to int
    if( pidOut )
        pidStream >> std::hex >> *pidOut;
    
    if( vidOut )
        vidStream >> std::hex >> *vidOut;
    return;
}


std::wstring 
IPod::getDeviceName( IWbemClassObject* const object ) const
{
    HRESULT result;
    VARIANT vtName;
    std::wstring retVal;

    result = object->Get( L"Name", 0, &vtName, 0, 0 );
    if( FAILED( result ) )
    {
        LOGL( 3, "Error getting Name from WMI: 0x" << std::hex << result );
        retVal = L"unknown";
    }
    else
    {
       retVal = std::wstring( _bstr_t( vtName.bstrVal ) );
       VariantClear( &vtName );
    }
    return retVal;
}


bool 
IPod::hasTimeoutExpired( DWORD timeoutValue )
{
    if( ( ::GetTickCount() - m_connectionTickCount ) > timeoutValue )
    {
        return true;
    }
    return false;
}


void //static
IPod::tokenize( const std::wstring & str, 
                      std::vector<std::wstring> & tokens, 
                      const std::wstring & delimiters /* = L" " */ )
{
    // Skip delimiters at beginning.
  std::wstring::size_type last_pos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
  std::wstring::size_type pos = str.find_first_of(delimiters, last_pos);
  while (std::wstring::npos != pos || std::wstring::npos != last_pos)
  {
        // Found a token, add it to the vector.
      tokens.push_back(str.substr(last_pos, pos - last_pos));
        // Skip delimiters.  Note the "not_of"
      last_pos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
      pos = str.find_first_of(delimiters, last_pos);
  }
}


void
IPod::populateMountPoint( IWbemClassObject *device )
{
    char driveLetter = NULL;
    int i;
    LOGL( 3, "Searching for iPod mountpoint.." );
    for( i = 0; i < 100 && driveLetter == NULL; i++ )
    {
        driveLetter = getDriveLetterFromPnPEntity( device );
        Sleep( 100 );
    }
    if( driveLetter == NULL )
    {
        LOGL( 3, "Error: could not determine mount point of connected iPod after 10 seconds of waiting." );
    } else
    {
        m_mountPoint = driveLetter;
        m_mountPoint += ":";
        LOGL( 3, "Mount point " << m_mountPoint << " found after" << ( (float)i / 10.0f) << "seconds" );
    }
}


char 
IPod::getDriveLetterFromPnPEntity( IWbemClassObject* device ) const
{
    char driveLetter = NULL;

    std::wstring query = L"select * from Win32_DiskDrive where PNPDeviceID LIKE '%";
    query += m_serial;
    query += L"%'";

    IEnumWbemClassObject* enumerator;

    HRESULT result = m_wmiServices->ExecQuery(
                         bstr_t("WQL"),
                         bstr_t(query.c_str()),
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                         NULL,
                         &enumerator );

    if (FAILED(result))
    {
        LOGL( 3, "ExecQuery Error code = 0x" 
            << std::hex << result );
        return NULL;
    }

    ULONG returnCount;

    IWbemClassObject* diskDrive;
    enumerator->Next( WBEM_INFINITE,
                      1,
                      &diskDrive,
                      &returnCount );

    if( returnCount > 0 )
    {
        driveLetter = getDriveLetterFromDiskDrive( diskDrive );
        diskDrive->Release();
    }
    enumerator->Release();
    return driveLetter;
}


char 
IPod::getDriveLetterFromDiskDrive( IWbemClassObject* diskDrive ) const
{
    char driveLetter = NULL;
    VARIANT vtName;
    diskDrive->Get( L"Name", 0, &vtName, 0, 0 );

    std::wstring query = L"Associators Of {Win32_DiskDrive.DeviceID=\"";
    query += vtName.bstrVal;
    query += L"\"} where AssocClass = Win32_DiskDriveToDiskPartition";

    size_t pos = 0;
    for( pos = query.find(L"\\", pos); pos != query.npos ; pos = query.find(L"\\", pos) )
    {
        query.replace(pos, 1, L"\\\\");
        pos += 2;
    }

    IEnumWbemClassObject* diskPartitionEnum;

    HRESULT result = m_wmiServices->ExecQuery(
                         bstr_t("WQL"),
                         bstr_t(query.c_str()),
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                         NULL,
                         &diskPartitionEnum );

    if (FAILED(result))
    {
        LOGL( 3, "ExecQuery Error code = 0x" 
            << std::hex << result );
        return NULL;
    }

    ULONG diskEnumReturnCount;
    IWbemClassObject* Partition;
    diskPartitionEnum->Next( WBEM_INFINITE,
                             1,
                             &Partition,
                             &diskEnumReturnCount );

    if( diskEnumReturnCount > 0 )
    {
        driveLetter = getDriveLetterFromPartition( Partition );
        Partition->Release();
    }

    diskPartitionEnum->Release();
    return driveLetter;
}

char 
IPod::getDriveLetterFromPartition( IWbemClassObject* partition ) const
{
    char driveLetter = NULL;
    VARIANT vtPartitionID;
    partition->Get( L"DeviceID",
                           0,
                           &vtPartitionID,
                           0,
                           0 );


    std::wstring query = L"associators of {Win32_DiskPartition.DeviceID=\"";
    query += vtPartitionID.bstrVal;
    query += L"\"} where AssocClass = Win32_LogicalDiskToPartition";

    IEnumWbemClassObject* logicalDiskEnum;

    HRESULT result = m_wmiServices->ExecQuery(
                     bstr_t("WQL"),
                     bstr_t(query.c_str()),
                     WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                     NULL,
                     &logicalDiskEnum );

    if (FAILED(result))
    {
        LOGL( 3, "ExecQuery Error code = 0x" 
            << std::hex << result );
        return NULL;
    }

    ULONG logicalDiskEnumReturnCount;

    IWbemClassObject* logicalDisk;
    logicalDiskEnum->Next( WBEM_INFINITE,
                             1,
                             &logicalDisk,
                             &logicalDiskEnumReturnCount );

    if( logicalDiskEnumReturnCount > 0 )
    {
        VARIANT vtDriveLetter;
        logicalDisk->Get( L"DeviceID",
                          0,
                          &vtDriveLetter,
                          0,
                          0);

        //This returns the first byte of the first wchar_t in the string
        //which will be the drive letter of the drive unless microsoft go
        //and do something stupid like enable unicode drive letters!
        driveLetter = (reinterpret_cast<char*>( vtDriveLetter.pbstrVal ))[0];
        logicalDisk->Release();
    }
    logicalDiskEnum->Release();
    return driveLetter;
}

void
IPod::populateIPodManual()
{
    std::string iTunesPrefsPath = m_mountPoint + "\\iPod_Control\\iTunes\\iTunesPrefs";
    FILE* fp = fopen( iTunesPrefsPath.c_str(), "r" );

    try 
    {
        if (!fp) throw "Couldn't open";
        int r = fseek( fp, 10, SEEK_SET );
        if (r != 0) throw "Couldn't seek to byte 10";
        int c = fgetc( fp );
        m_manualMode = (c == 0);
    }
    catch (const char* message)
    {
        LOGL( 2, message << " `" << iTunesPrefsPath << "'" );
    }

    if( fp )
        fclose( fp );

    LOGL( 3, "Sync mode detected: " << ( m_manualMode ? "Manual" : "Automatic" ) );
}


void 
IPod::populateIPhoneMobileScrobblerAndManualMode()
{
    WCHAR appData[ MAX_PATH ];
    
    HRESULT result = ::SHGetFolderPathW( NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appData );
    if( result != S_OK )
    {
        LOGL( 3, "Could not get CSIDL_APPDATA location - presuming iPhone is in automatic sync mode and mobile scrobbler is NOT installed." );
        return;
    }
    
    std::wstring backupPath = appData;
    backupPath += L"\\Apple Computer\\MobileSync\\Backup\\";
    backupPath += m_serial;
    backupPath += L"\\";

    std::wstring infoPlistPath = backupPath + L"Info.plist";

    waitForIpodBackup( infoPlistPath );

    m_mobileScrobblerInstalled = queryMobileScrobblerInstalled( backupPath );
    m_manualMode = queryIPhoneManual( infoPlistPath );

}


void 
IPod::waitForIpodBackup( const std::wstring& backupPath ) const
{
    struct _stat infoPlistStat;
    time_t curTime = time( NULL );
    time_t infoPlistModTime = 0;

    int i = 0;
    for( i = 0; infoPlistModTime < curTime && i < (5 * 60); i++ )
    {
        Sleep( 1000 );       
        if( _wstat( backupPath.c_str(), &infoPlistStat ) )
        {
            //TODO: error handling pls
        }

        infoPlistModTime = infoPlistStat.st_mtime;
    }

    if( infoPlistModTime < curTime )
        LOGL( 3, "Waiting for ipod backup has timed out - "
                 "Presuming that settings haven't changed since last backup." );
}


bool 
IPod::queryIPhoneManual( const std::wstring& infoPlistPath ) const
{
    char manualMode;
    try
    {
        std::ifstream plistStream( infoPlistPath.c_str() );
        if( plistStream.fail() )
            throw std::string( "Could not open info.plist backup file." );

        Plist plist( plistStream );
        Element iTunesPrefs = plist[0][ "iTunes Files" ][ "iTunesPrefs" ];
        if( iTunesPrefs.getDataLength() < 10 )
            throw std::string( "iTunesPrefs data looks too short" );

        manualMode = iTunesPrefs.getData()[10];
    }
    catch( const std::string& e )
    {
        LOGL( 3, "Could not determine whether iPod is in manual / automatic mode error: " << e
                  << ". Presuming automatic mode.");
        manualMode = 1;
    }
    return (manualMode == 0);
}


bool 
IPod::queryMobileScrobblerInstalled( const std::wstring& backupPath ) const
{

    std::wstring searchPath = backupPath + L"*.mdbackup";

    WIN32_FIND_DATAW fileInfo;
    HANDLE dirHandle = ::FindFirstFileW( searchPath.c_str(), &fileInfo );

    if( dirHandle == INVALID_HANDLE_VALUE )
    {
        LOGL( 3, "Could not search the backup folder for mdbackup files - presuming Mobile scrobbler is NOT installed." );
        return false;
    }

    do{
        std::wstring curPath = backupPath + fileInfo.cFileName;
        if( isMobileScrobblerPlist( curPath ) )
            return true;
    }while( ::FindNextFileW( dirHandle, &fileInfo ) != 0 );
    
    
    FindClose( dirHandle );
    return false;
}


bool 
IPod::isMobileScrobblerPlist( const std::wstring& path ) const
{
    std::ifstream fin( path.c_str() );
    fin.seekg( 0, std::ios_base::end );
    int length = fin.tellg();
    fin.seekg( 0, std::ios_base::beg );
    char* buffer = new char[ length ];
    fin.read( buffer, length );

    bool retval = false;
    if(strstr (buffer, "org.c99.MobileScrobbler.plist" ))
        retval = true;

    delete[] buffer;
    return retval;
}