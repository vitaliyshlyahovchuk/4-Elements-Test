#include "stdafx.h"
#include "GameInfo.h"

#include "Game.h"
#include "resource.h"
#include "MyApplication.h"
#include "Platform/win.h"

#include <ShellAPI.h>
#include <shlobj.h>
#include <aclapi.h>
#include <direct.h>
#include <dbghelp.h>

#define USE_INI_PATH
//#define GAME_PATH

//#include "direct.h" // directory stuff

#include <dbghelp.h>

const std::string MYWINDOW_CAPTION		("4 Elements Freemium");
const std::string MYWINDOW_CLASS_NAME	("4ElementsFreemiumWindowClass");
const std::string MYSETTINGS_REG_KEY	("Software\\Playrix Entertainment\\4ElementsFreemium");


utils::SingleInstance singleInstance("4Elements2-3ED026B7-84BC-4799-97D1-B50FA53F8393", MYWINDOW_CLASS_NAME, MYWINDOW_CAPTION);

bool miniDumpCreated = false;	//был ли создан минидамп (случалось так, что за один вылет создавалось больше одного минидампа)

IPoint GetResolutionSize(IPoint game_size)
{
	//Функция заглушка для Windows, ретина определяется на основании входящего разрешения, на ios - по другому
	float proportions = float(game_size.x)/game_size.y;
	bool is_retina = false;
	if(proportions > 0.74)
	{
		//iPad
		//IPoint(768, 1024); //0.74
		is_retina = float(game_size.x)/768.f > 1.99f;
	}else if(proportions > 0.6)
	{
		//iPhone
		//IPoint(640, 960); //0.66
		is_retina = float(game_size.x)/640.f > 1.99f;
	}else if(proportions > 0.56)
	{
		//iPhone 5
		//IPoint(640, 1136); //0.56
		is_retina = float(game_size.x)/640.f > 1.99f;
	}	
	gameInfo.setGlobalBool("is_retina", is_retina);
	if(is_retina)
	{
		game_size.x /= 2;
		game_size.y /= 2;
	}
	return game_size;
}

int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
	if (miniDumpCreated)
	{
		return -10;
	}
	miniDumpCreated = true;

	EXCEPTION_RECORD* pRecord = pExceptionPointers->ExceptionRecord;

	if (pRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		Log::Fatal("Access violation!");
	}
	else if (pRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
	{
		Log::Fatal("In page error!");
	}
	else if (pRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO)
	{
		Log::Fatal("Divide by zero!");
	}
	else if (pRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		Log::Fatal("Stack overflow!");
	}
	else
	{
		Log::Fatal("Unhandled exception!");
	}

	Log::Info("Exception information");
	Log::Info("Exception adress : "+utils::lexical_cast(pRecord->ExceptionAddress));
	Log::Info("Exception flags : "+utils::lexical_cast(pRecord->ExceptionFlags));
	Log::Info("Exception code : "+utils::lexical_cast(pRecord->ExceptionCode));

	CHAR szPath[MAX_PATH];
	CHAR szFileName[MAX_PATH];
	CHAR* szAppName = "CRASHDUMP";
	CHAR* szVersion = "v1.0";
	DWORD dwBufferSize = MAX_PATH;
	HANDLE hDumpFile;
	SYSTEMTIME stLocalTime;
	MINIDUMP_EXCEPTION_INFORMATION ExpParam;

	std::string path = File::GetSpecialFolderPath(SpecialFolder::LocalData);
	GetLocalTime( &stLocalTime );
	szPath[0] = '\0';
	strncpy_s(szPath, 259, path.c_str(), path.length());
	sprintf_s( szFileName, "%s%s", szPath, szAppName );
	CreateDirectory( szFileName, NULL );

	sprintf_s( szFileName, "%s%s\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp", 
		szPath, szAppName, szVersion, 
		stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, 
		stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond, 
		GetCurrentProcessId(), GetCurrentThreadId());
	hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	ExpParam.ThreadId = GetCurrentThreadId();
	ExpParam.ExceptionPointers = pExceptionPointers;
	ExpParam.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
		hDumpFile, MiniDumpWithFullMemory, &ExpParam, NULL, NULL);

	CloseHandle(hDumpFile);

	return EXCEPTION_EXECUTE_HANDLER;
}

// Флаг, если не из bat, крешдампы не создаем (помогает в release ловить баги) //
bool isNeedDump = false;

int APIENTRY WinMainStart(	HINSTANCE hInstance,
						  HINSTANCE hPrevInstance,
						  LPSTR     lpCmdLine,
						  int       nCmdShow)
{
#ifdef TO_FIND_MEMORY_BUG
	_CrtSetDbgFlag(
    _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |
    //_CRTDBG_CHECK_ALWAYS_DF |
    _CRTDBG_DELAY_FREE_MEM_DF
	);
#endif

	Core::Application::SETTINGS_REG_KEY = MYSETTINGS_REG_KEY;
	Core::Application::APPLICATION_NAME = MYWINDOW_CAPTION;
	Core::Application::WINDOW_CLASS_NAME = MYWINDOW_CLASS_NAME;

	MyApplication::PreInit();

	MyApplication App(hInstance, nCmdShow, false);

	gameInfo.LoadFromFile();


	App.GAME_CONTENT_WIDTH =  MyApplication::GAME_WIDTH;
	App.GAME_CONTENT_HEIGHT = MyApplication::GAME_HEIGHT;

	App._saveGfxFileInfo = false;

	App.Init(true); 

	App.showFps = true;
	App.maxFps = 2000;
	Render::device.SetVSyncState(0);

	App.Start();
	App.MarkAsShuttingDown();

	App.Release();
	App.ShutDown();

	Log::log.RemoveAllSinks();
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPSTR     lpCmdLine,
					 int       nCmdShow)
{
#ifndef _DEBUG
	__try
#endif
	{
		WinMainStart(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
#ifndef _DEBUG
	__except(GenerateDump(GetExceptionInformation()))
	{
	}
#endif
}


void UpdateGameInfoHash(std::string data)
{
}

bool hasModificationGameInfo(std::string data)
{
	return false;
}