// comtest.cpp : Defines the entry point for the console application.
//

#include "ITunesComThread.h"
#include "common/logger.h"

#include <string>
#include <cstdio>

#include <atlbase.h>
#include <tchar.h>

//class ComtestModule : public CAtlExeModuleT< ComtestModule >
//{
//};

//ComtestModule _AtlModule;

int _tmain(int argc, _TCHAR* argv[])
{
    Logger::GetLogger().Init( L"test.log" );

    printf("main");
    
    ITunesComThread t;

    //Sleep( 1000 * 1000 );

    getchar();

	return 0;
}

