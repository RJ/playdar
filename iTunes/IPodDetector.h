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

#ifndef IPOD_DETECTOR_H
#define IPOD_DETECTOR_H

#include "common/logger.h"
#include "Moose.h"
#include "IPod.h"

#include <string>
#include <map>
#include <vector>

#ifdef WIN32
    #include <windows.h>
#else
    #include <IOKit/IOKitLib.h>
    #include "pthread.h"

    #include <CoreServices/CoreServices.h>
    #include <Carbon/Carbon.h>    
#endif


/** @author Jono Cole <jono@last.fm> 
  * @brief launches twiddly for diffing when an iPod / iPhone is unplugged
  */
class IPodDetector
{
public:
    IPodDetector();
    ~IPodDetector();

private:
    // NOTE inline because there is no IPodDetector.cpp
    void
    startTwiddlyWithFlag( const COMMON_STD_STRING& args )
    {
        if ( !Moose::isTwiddlyRunning() )
        {
            Moose::exec( Moose::twiddlyPath(), args );
        }
    }

    // NOTE inline because there is no IPodDetector.cpp
    void 
    startTwiddlyWithIpodSerial( const COMMON_STD_STRING& serial, char* trigger = "unknown" )
    {
        std::map< COMMON_STD_STRING, IPod*>::iterator it = m_ipodMap.find( serial );
        if( it != m_ipodMap.end() )
        {
            LOG( 3, "Ipod scrobbling triggered by " << trigger << " method." );
            IPod* iPod = it->second;
            startTwiddlyWithFlag( iPod->twiddlyFlags() );
            m_ipodMap.erase( serial );
            delete iPod;
        }
    }

    std::map< COMMON_STD_STRING, IPod*> m_ipodMap;
    
    void notifyIfUnknownIPod( IPod* ipod );

  #ifdef WIN32
    void threadMain();

    static unsigned CALLBACK threadEntry( LPVOID lpParam );
    static LRESULT CALLBACK WindowProc(  HWND hwnd,
                                         UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam
                                     );
    
    static std::map< HWND, IPodDetector* > s_hwndMap;
    HWND m_deviceDetectionWnd;

    bool initializeWMI();
    void shutdownWMI();

    /** Check if the device still exists (determined using the serial number and WMI) */
    bool doesIpodExist( IPod* ipod );

    void checkConnectedDevices();

    bool queryCurrentlyConnectedDevices( struct IEnumWbemClassObject** enumerator );
    bool queryByDeviceID( const std::string& deviceId, IEnumWbemClassObject** enumerator );

    void runDetectionEventLoop();

    void onDeviceConnected( struct IWbemClassObject* const device );
    void onDeviceRemoved();

    std::string getDBTDeviceInfo( const LPARAM lparam ) const;

    bool m_shutdown;

    HANDLE m_thread;

    struct IWbemLocator *m_wmiLocator;
    struct IWbemServices *m_wmiServices;

  #else
    bool setupDetection();

    /// callbacks
    static void* threadEntry( void* param );
    static void onIPhoneDetected( void*, io_iterator_t newIterator );
    static void onUsbIPodDetected( void*, io_iterator_t newIterator );
    static void onDeviceRemoved( void*, io_iterator_t newIterator );
    static void onFireWireDetected( void*, io_iterator_t newIterator );
    static void onFireWireRemoved( void*, io_iterator_t newIterator );
    static void onDeviceNodeAdded( void*, io_iterator_t newIterator );
    static void onDeviceNodeRemoved( void*, io_iterator_t newIterator );
    
    static OSStatus onMountStateChanged(  EventHandlerCallRef handlerCallRef,
                                          EventRef event,
                                          void* userData );

    
    static void onSyncTimerFire( CFRunLoopTimerRef timer, void *info );
    static void onSyncTimerRelease( const void *info );

    static void onQuit( void* param );
    
    void startSyncTimer( class IPod* const ipod );
    
    std::string getMountPoint( const std::string& deviceName ) const;

    CFRunLoopSourceRef m_quitSource;
    
    pthread_t m_threadId;
    
    bool m_detectorStarted;
    
    bool m_threadError;
    
  #endif
};

#endif //IPOD_DETECTOR_H
