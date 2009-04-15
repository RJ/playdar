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
#include "common/c++/Logger.h"
#include "Moose.h"
#include "IPod.h"
#include <cassert>

#include <process.h> // for beginthreadex
#include <iostream>
#include <sstream>

#include <Windows.h>
#include <Dbt.h>
#include <SetupAPI.h>

#include <comdef.h>
#include <Wbemidl.h>

using namespace std;

std::map< HWND, IPodDetector* > IPodDetector::s_hwndMap;

static const DWORD kTimeOutInterval = 5 * 60 * 1000;  // 5 minute timeout before sync is forced

//The following strings are used in the notification query
#define IPHONE_ITOUCH_NAME "Apple Mobile Device USB Driver"
#define IPOD_NAME "USB Mass Storage Device"
#define IPOD_NANO_NAME "Apple iPod USB Driver"
#define IPOD_FIREWIRE_NAME "Apple Computer_ Inc. iPod IEEE 1394 SBP2 Device"

#define VID_STRING "%VID_05AC%"
#define LONG_VID_STRING "%APPLE_COMPUTER__INC%"


IPodDetector::IPodDetector()
             :m_shutdown( false ),
              m_wmiLocator( NULL ),
              m_wmiServices( NULL )
{
    unsigned int threadId;
    m_thread = (HANDLE)_beginthreadex( NULL,
                                       0,
                                       IPodDetector::threadEntry,
                                       this,
                                       0,
                                       &threadId );

    LOG( 3, "Starting iPod detection" );
}


IPodDetector::~IPodDetector()
{
    m_shutdown = true;

    //Wake up the event loop
    ::SendMessage( m_deviceDetectionWnd, WM_NULL, 0, 0 );
    
    DWORD threadResult = WaitForSingleObject( m_thread, 5000 );
    if( threadResult == WAIT_TIMEOUT )
        LOGL( 3, "Error: the IPodDetector shutdown code failed to complete after 5 seconds." );
}


unsigned CALLBACK //static
IPodDetector::threadEntry( LPVOID lpParam )
{
    IPodDetector* ipd = static_cast<IPodDetector*>(lpParam);
    ipd->threadMain();
    return 0;
}


void
IPodDetector::threadMain()
{
    if( !initializeWMI() )
    {
        return;
    }

    checkConnectedDevices();

    runDetectionEventLoop();
       
    LOGL( 3, "Cleaning up iPod Detector" );
   
    shutdownWMI();

    return;
	
}


void 
IPodDetector::runDetectionEventLoop()
{
    WNDCLASSEX wndClass;
    wndClass.cbSize = sizeof( wndClass );
    wndClass.style = NULL;
    wndClass.lpfnWndProc = IPodDetector::WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = GetModuleHandle(NULL);
    wndClass.hIcon = NULL;
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = NULL;
    wndClass.lpszMenuName = "";
    wndClass.lpszClassName = "DeviceDetectionClass";
    wndClass.hIconSm = NULL;

    ATOM regClassResult = RegisterClassEx( &wndClass );
    
    m_deviceDetectionWnd = CreateWindow(  "DeviceDetectionClass",
                                          "DeviceDetectionWindow",
                                          NULL,
                                          0,
                                          0,
                                          0,
                                          0,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);


    s_hwndMap[ m_deviceDetectionWnd ] = this;


    DEV_BROADCAST_DEVICEINTERFACE filter;
    ZeroMemory( &filter, sizeof( filter ) );
    filter.dbcc_size = sizeof( filter );
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    HDEVNOTIFY deviceNotifierHandle = RegisterDeviceNotification(
        m_deviceDetectionWnd,
        reinterpret_cast<void*>( &filter ),
        DEVICE_NOTIFY_WINDOW_HANDLE |
        // This next parameter causes notifications to be sent for all devices,
        // regardless of the dbcc_classguid member of filter. However, it's not
        // supported by Win 98/2000 so better use a sensible class anyway.
        0x00000004 /* == DEVICE_NOTIFY_ALL_INTERFACE_CLASSES argh! */ );

    if( !deviceNotifierHandle )
         LOG( 3, "RegisterDeviceNotification Error: " << GetLastError() );


    BOOL messageVal = 1;    //initialize to non-zero value
    MSG message;
    while( messageVal != 0 && !m_shutdown )
    {
        if( PeekMessage( &message, m_deviceDetectionWnd, 0, 0, PM_REMOVE ) > 0 )
        {
            if( messageVal == -1 )
            {
                LOGL( 3, "GetMessage error: " << GetLastError() );
            }
            else
            {
                TranslateMessage( &message );
                DispatchMessage( &message );
            }
        }

        Sleep( 100 ); //Think of the poor CPU cycles
        
        if( m_ipodMap.empty() )
            continue;

        //Check to see if any of the connected iPods have timedout
        std::map< std::wstring, IPod* >::iterator iter;
        for( iter = m_ipodMap.begin(); iter != m_ipodMap.end(); )
        {
            IPod* ipod = iter->second;

            /* This looks wierd but we must increment the pointer before the
             * startTwiddlyWithIpodSerial method erases the ipod and invalidates
             * the previous pointer (Norman will tell you all about this) */
            ++iter;
            if( ipod->hasTimeoutExpired( kTimeOutInterval ) )
            {
                startTwiddlyWithIpodSerial( ipod->serial(), "timeout" );
            }
        }
    }

}


LRESULT CALLBACK  //static
IPodDetector::WindowProc(  HWND hwnd,
                           UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam
                         )
{
    IPodDetector* ipd = s_hwndMap[ hwnd ];

    switch( uMsg )
    {
        case WM_DEVICECHANGE:
        {
            HRESULT result = PostMessage( hwnd, WM_USER, wParam, lParam );
            if( FAILED( result ) )
                LOGL( 3, "error: PostMessage WM_USER failed: " << ::GetLastError() )

        }
        break;

        case WM_USER:
            switch( wParam )
            {
                case DBT_DEVICEARRIVAL:
                {
                    std::string deviceId = ipd->getDBTDeviceInfo( lParam );
                    LOGL( 3, "Device arrived - id: " << deviceId )
                    IEnumWbemClassObject* enumerator;
                    ipd->queryByDeviceID( deviceId, &enumerator );
                    
                    IWbemClassObject* object;
                    ULONG retCount;

                    int count = 0;
                    while( enumerator )
                    {
                        enumerator->Next( WBEM_INFINITE,
                                          1,
                                          &object,
                                          &retCount );

                        if( 0 == retCount )
                            break;

                        ipd->onDeviceConnected( object );
                        object->Release();
                        count++;
                    }
                    if( count > 0 )
                    {
                        LOGL( 3, count << " ipods found with this deviceId" );
                    }
                    else
                    {
                        LOGL( 3, "no ipods found with this deviceid - presumeably a non-ipod has been plugged in!" )
                    }
                    enumerator->Release();
                }
                break;

                case DBT_DEVICEREMOVECOMPLETE:
                {
                    //std::string deviceId = ipd->getDBTDeviceInfo( lParam );
                    LOGL( 3, "DBT device removal detected - checking for disconnected iPod.." );
                    ipd->onDeviceRemoved();
                }
                break;

                default:
                {
        //            LOGL( 3, "Unknown WM_DEVICECHANGE wParam: " << wParam );
                }
                break;
            }
        break;

        default:
            return DefWindowProc( hwnd, uMsg, wParam, lParam );
        break;
    }
    return 0;
}


std::string 
IPodDetector::getDBTDeviceInfo( const LPARAM lParam ) const
{
    std::string retVal = "unknown";
    PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;

    if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
    {
        PDEV_BROADCAST_DEVICEINTERFACE lpDevIf = (PDEV_BROADCAST_DEVICEINTERFACE)lpdb;

        /* The dbcc_name needs to be copied into another memory location as
         * the SetupDiCreateDeviceInfoList function seems to clear the lpdb memory
         * (this seems to only happen with firewire devices *sigh* ) */

        char* dbcc_name = new char[ strlen( lpDevIf->dbcc_name ) + 1 ];
        strncpy( dbcc_name, lpDevIf->dbcc_name, strlen( lpDevIf->dbcc_name ) + 1 );

        HDEVINFO devInfoList = SetupDiCreateDeviceInfoList( NULL, NULL );
        
        SP_DEVICE_INTERFACE_DATA devInterfaceData;
        ZeroMemory( &devInterfaceData, sizeof( devInterfaceData ) );
        devInterfaceData.cbSize = sizeof( devInterfaceData );
        BOOL b = SetupDiOpenDeviceInterface( devInfoList,
                                             dbcc_name,
                                             0,
                                             &devInterfaceData );
                                    
        delete[] dbcc_name;
        
        DWORD required;
        SP_DEVICE_INTERFACE_DETAIL_DATA devDetailData;
        ZeroMemory( &devDetailData, sizeof( devDetailData ) );
        devDetailData.cbSize = sizeof( devDetailData );

        
        SP_DEVINFO_DATA devInfoData;
        ZeroMemory( &devInfoData, sizeof( devInfoData ) );
        devInfoData.cbSize = sizeof( devInfoData );
        b = SetupDiGetDeviceInterfaceDetail( devInfoList,
                                         &devInterfaceData,
                                         &devDetailData,
                                         sizeof( devDetailData ),
                                         &required,
                                         &devInfoData );
       
        TCHAR deviceId[1000];
        b = SetupDiGetDeviceInstanceId( devInfoList,
                                        &devInfoData,
                                        deviceId,
                                        1000,
                                        NULL );

        retVal = deviceId; 
    }
    return retVal;
}


void 
IPodDetector::onDeviceConnected( IWbemClassObject* const device )
{
    IPod* iPod = IPod::newFromUsbDevice( device, m_wmiServices );

    if( !iPod )
        return;

    notifyIfUnknownIPod( iPod );

    if( iPod->isMobileScrobblerInstalled() )
    {
        LOGL( 3, "Ignoring this iPhone / iPod touch as mobile scrobbler has been detected!" );
        return;
    }

    m_ipodMap[ iPod->serial() ] = iPod;

    if( iPod->isManualMode() )
    {
        LOGWL( 3, L"Manual iPod detected launching twiddly..:" << iPod->serial() );
        startTwiddlyWithIpodSerial( iPod->serial(), "manual" );
    }
    else
    {
        LOGWL( 3, L"Device connected in automatic sync mode: " << iPod->serial() );
    }
}


void
IPodDetector::notifyIfUnknownIPod( IPod* ipod )
{
    std::wstring const key = UNICORN_HKEY L"\\device\\" +
                             ipod->device() + L"\\" +
                             ipod->serial();

    HKEY h = NULL;
    LONG r = RegOpenKeyExW( HKEY_CURRENT_USER,
                            key.c_str(),
                            0,              // reserved
                            KEY_ALL_ACCESS, // access mask
                            &h );
    
    if ( r != ERROR_SUCCESS )
    {
        Moose::exec( Moose::applicationPath(), 
                     L"--ipod-detected" );
        
        r = RegCreateKeyExW( HKEY_CURRENT_USER,
                             key.c_str(),
                             0,                       // reserved, must be 0
                             NULL,                    // class, ignore
                             REG_OPTION_NON_VOLATILE, // saves key on exit
                             KEY_ALL_ACCESS,          // access mask. TODO: test this as non-admin
                             NULL,                    // security attrs, NULL = default, inherit from parent
                             &h,
                             NULL );
    }

    // Internet says this is safe, even if NULL, or after failed function calls
    RegCloseKey( h );
}


void 
IPodDetector::onDeviceRemoved()
{
    bool iPodFound = false;
    std::map< std::wstring, IPod* >::iterator iter;

    for( iter = m_ipodMap.begin(); iter != m_ipodMap.end();)
    {
        IPod* curIpod = iter->second;
        iter++; //Stop the iterator from being invalidated
        if( !doesIpodExist( curIpod ) )
        {
            iPodFound = true;
            LOGWL( 3, L"Ipod: " << curIpod->serial() << L" removed." );
            startTwiddlyWithIpodSerial( curIpod->serial(), "unplug" );
        }
    }
    if( !iPodFound )
        LOGL( 3, "Could not find any disconnected iPods - a non-ipod device must have been removed" );
}


bool 
IPodDetector::initializeWMI()
{
    HRESULT result;

    // Initialize COM.

    result =  CoInitializeEx(0, COINIT_APARTMENTTHREADED); 
    if (FAILED(result))
    {
        LOGL( 3, "Failed to initialize COM library. Error code = 0x" 
            << hex << result );
        return false;
    }

    
    // Obtain the initial locator to WMI

    result = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, 
        (LPVOID *) &m_wmiLocator);
 
    if (FAILED(result))
    {
        LOGL( 3, "Failed to create IWbemLocator object."
            << " Err code = 0x"
            << hex << result );
        CoUninitialize();
        return false;
    }

    // Connect to WMI through the IWbemLocator::ConnectServer method

	
    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer m_services
    // to make IWbemServices calls.
    result = m_wmiLocator->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &m_wmiServices           // pointer to IWbemServices proxy
         );
    
    if (FAILED(result))
    {
        LOGL( 3, "Could not connect to WMI. Error code = 0x" 
             << hex << result );
        m_wmiLocator->Release();     
        CoUninitialize();
        return false;
    }

    LOGL( 3, "Connected to ROOT\\CIMV2 WMI namespace" );


    // Set security levels on the proxy

    result = CoSetProxyBlanket(
       m_wmiServices,               // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(result))
    {
        LOGL( 3, "Could not set proxy blanket. Error code = 0x" 
            << hex << result );
        m_wmiServices->Release();
        m_wmiLocator->Release();     
        CoUninitialize();
        return false;
    }
    return true;
}


bool 
IPodDetector::queryByDeviceID( const std::string& deviceId, IEnumWbemClassObject** enumerator )
{
   HRESULT result;

   std::string query = "SELECT * FROM Win32_PnPEntity "
                       "WHERE DeviceID LIKE '";
   query += deviceId;
   query += "' AND "
            " (Name = '" IPHONE_ITOUCH_NAME "' OR "
            "  Name = '" IPOD_NAME "' OR "
            "  Name = '" IPOD_FIREWIRE_NAME "' OR "
            "  Name = '" IPOD_NANO_NAME "') AND "
            " (DeviceID LIKE '" VID_STRING "' OR "
            "  DeviceID LIKE '" LONG_VID_STRING "')";

   size_t pos = 0;
   for( pos = query.find("\\", pos); pos != query.npos ; pos = query.find("\\", pos) )
   {
       query.replace(pos, 1, "\\\\");
       pos += 2;
   }

   result = m_wmiServices->ExecQuery(
        bstr_t("WQL"),
        bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        enumerator
    );

    if (FAILED(result))
    {
        LOGL( 3, "ExecQuery Error code = 0x" 
            << hex << result );
        return false;
    }
    return true;
}


bool 
IPodDetector::queryCurrentlyConnectedDevices( IEnumWbemClassObject** enumerator )
{
    HRESULT result;

    result = m_wmiServices->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_PnPEntity "
               "WHERE  (Name = '" IPHONE_ITOUCH_NAME "' OR "
               "        Name = '" IPOD_NAME "' OR "
               "        Name = '" IPOD_FIREWIRE_NAME "' OR "
               "        Name = '" IPOD_NANO_NAME "') AND "
               "       (DeviceID LIKE '" VID_STRING "' OR "
               "        DeviceID LIKE '" LONG_VID_STRING "')"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        enumerator
    );

    if (FAILED(result))
    {
        LOGL( 3, "ExecQuery Error code = 0x" 
            << hex << result );
        return false;
    }
    return true;
}


void
IPodDetector::checkConnectedDevices()
{
    IEnumWbemClassObject* currentDevicesEnumerator = NULL;
    queryCurrentlyConnectedDevices( &currentDevicesEnumerator );

    IWbemClassObject* device;
    ULONG returnCount = 0;

    while( currentDevicesEnumerator )
    {
        currentDevicesEnumerator->Next( WBEM_INFINITE,
                                        1,
                                        &device,
                                        &returnCount );
        if( 0 == returnCount )
        {
            break;
        }
        if( returnCount > 0 )
        {
            LOGL( 3, "Detected pre-connected iPod" );
            onDeviceConnected( device );
        }
        device->Release();
    }
    currentDevicesEnumerator->Release();
}


void 
IPodDetector::shutdownWMI()
{
    m_wmiServices->Release();
    m_wmiLocator->Release();
    CoUninitialize();
}


bool 
IPodDetector::doesIpodExist(IPod *ipod)
{
    IEnumWbemClassObject* enumerator = NULL;
    std::stringstream output;

    for( size_t i = 0; i < ipod->serial().length(); ++i )
    {
        output << (char)ipod->serial().c_str()[i];
    }
    queryByDeviceID( output.str(), &enumerator );
    
    IWbemClassObject* object;
    ULONG count;

    enumerator->Next( WBEM_INFINITE, 1, &object, &count );
    
    enumerator->Release();

    if( count > 0 )
    {
        object->Release();
        return true;
    }
    return false;
}
