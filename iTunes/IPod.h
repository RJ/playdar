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

#ifndef IPOD_H
#define IPOD_H

#include <string>
#include <vector>
#include "common/logger.h"

#ifdef WIN32
    #define LFM_STRINGSTREAM std::wstringstream
#else
    #include <IOKit/IOKitLib.h>
    #include <CoreFoundation/CoreFoundation.h>
    #define LFM_STRINGSTREAM std::stringstream
#endif


/** @author Jono Cole <jono@last.fm> 
  * @brief stores iPod / iPhone device information
  */  
    class IPod
    {
        public:
            const COMMON_STD_STRING twiddlyFlags() const;
            const COMMON_STD_STRING& serial() const { return m_serial; }
            COMMON_STD_STRING device() const;

            enum deviceType { unknown = 0,
                   iPod,
                   iTouch,
                   iPhone };

            bool isManualMode() const{ return m_manualMode; }
            bool isMobileScrobblerInstalled() const { return m_mobileScrobblerInstalled; }

            /** populates the m_manualMode bool 
              * NOTE possibly shouldn't be here */
            void populateIPodManual();
        
            #ifdef WIN32

                //Note: It's not ideal that the IPodDetector has to pass the IWbemServices pointer
                //      to the IPod class but it is necessary in order to make further WMI queries
                //      in this class. It should be safe however as no IPod instances should exist
                //      beyond the scope of the IPodDetector instance.
                static IPod* newFromUsbDevice( struct IWbemClassObject* device, 
                                               struct IWbemServices* wmiServices );

                /** get the serial, pid and vid of a CIM_PnPEntity device
                  * ( pass NULL to any of the output parameters if not needed ) */
                static void getSerialVidPid( struct IWbemClassObject* device, 
                                             std::wstring *const serialOut = NULL, 
                                             int *const vidOut = NULL, 
                                             int *const pidOut = NULL );

                /** get the name of the CIM_PnPEntity device */
                std::wstring getDeviceName( struct IWbemClassObject* const object ) const;

                /** Check if the IPod is older than the timeout value (milliseconds) ) */
                bool hasTimeoutExpired( DWORD timeoutValue );

            #else //MAC:
                /** Utility method to get the usb serial number of a io_object_t device
                  * without having to create an object. */
                static bool getUsbSerial( io_object_t device, std::string* serialOut );
                static bool getFireWireSerial( io_object_t device, std::string* serialOut );

                static IPod* newFromUsbDevice( io_object_t device, bool isIPhone = true );
                static IPod* newFromFireWireDevice( io_object_t device );
                static io_object_t getBaseDevice( io_object_t device );
                
                void setMountPoint( const std::string& mountPoint ){ m_mountPoint = mountPoint; }
                void setDiskID( const std::string& diskID ){ m_diskID = diskID; }
                const std::string& diskID() const{ return m_diskID; }
            #endif

        private:
            IPod(): m_manualMode( false ), m_mobileScrobblerInstalled( false )
            {}
        
            COMMON_STD_STRING m_serial;
            int m_pid;
            int m_vid;
            
            bool m_manualMode;
            bool m_mobileScrobblerInstalled;

            enum { fireWire = 0,
                   usb } m_connectionType;

            deviceType m_type;

            std::string m_mountPoint;

            #ifdef WIN32
                static void tokenize( const std::wstring& str, 
                                      std::vector< std::wstring >& tokens, 
                                      const std::wstring& delimiters = L" " );

                DWORD m_connectionTickCount;

                struct IWbemServices* m_wmiServices;
                
                /** populates the m_mountPoint string querying using WMI */
                void populateMountPoint( IWbemClassObject *device );
                char getDriveLetterFromPnPEntity( IWbemClassObject* device ) const;
                char getDriveLetterFromDiskDrive( IWbemClassObject* diskDrive ) const;
                char getDriveLetterFromPartition( IWbemClassObject* partition ) const;

                /** populates the m_manualMode and m_mobileScrobblerInstalled bools */
                void populateIPhoneMobileScrobblerAndManualMode();

                /** Waits for iPod backup to complete (checking mod time of Info.plist
                  * or times out after 5 mins.
                  */
                void waitForIpodBackup( const std::wstring& backupPath ) const;

                /** Query if mobile scrobbler is installed by iterating through each
                  * file in the backupPath directory checking for the if it's a 
                  * mobilescrobbler plist file.
                  */
                bool queryMobileScrobblerInstalled( const std::wstring& backupPath ) const;

                /** Determines if a file is a backup of a mobilescrobbler plist file by 
                  * searching for the string "org.c99.MobileScrobbler.plist". 
                  */
                bool isMobileScrobblerPlist( const std::wstring& path ) const;

                /** Determines if the iPhone is in manual or automatic mode by
                  * reading the info.plist file and checking the iTunesPrefs section.
                  */
                bool queryIPhoneManual( const std::wstring& infoPlistPath ) const;

            #else //MAC:
                bool isOldIpod( io_object_t device );
                bool getDeviceId( io_object_t device, int* vid, int* pid );
                bool getDeviceFireWireId( io_object_t device, int* vid, int* pid );
                std::string m_diskID;

                
                bool isMobileScrobblerPlist( const char* path ) const;
                bool queryMobileScrobblerInstalled() const;
                bool queryIPhoneManual() const;
                bool waitForIpodBackup() const;
                CFDictionaryRef createDictionaryFromXML( CFStringRef filePath ) const;
            #endif
    };

#endif //IPOD_H
