#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

extern bool needLoadScreen;

#include "Core/Application.h"
#include "Core/Window.h"
#include <string>
#include "Flash/core/IPlaybackOperation.h"

class MyApplication
	: public Core::Application
{

public:
	// cтатические width и height, т.к. объектные Application не подходят - инициализируются поздно
	static int GAME_HEIGHT;
	static int GAME_WIDTH;

#ifdef ENGINE_TARGET_WIN32
	MyApplication(HINSTANCE hInstance, int nCmdShow, bool fullscreenMode);
#else
	MyApplication();
#endif

	virtual ~MyApplication();

	virtual void PreloadResources();
 
	virtual void LoadResources();
	void RealLoadResources();

	virtual void ScriptMap();

	virtual void RegisterTypes();

	virtual void DrawPause();

	virtual void PostDraw();

#ifdef ENGINE_TARGET_WIN32
	virtual void DrawFps();
#else
	virtual void DebugDraw();
	virtual void BaseDraw();
#endif
	
	void SetParseLoaded();
	
	void Update(float dt);

	// Указатель на единcтвенный экземпляр
	static MyApplication *GetInstance();

	// Уcтановить/проверить cоcтояние завершения приложения
	void MarkAsShuttingDown();
	bool IsShuttingDown() const;
	void Release();
    
    static void PreInit(const std::string &workDir = "");
	static void InitAudioSession();
	static bool IsOtherAudioPlaying();

private:
    
	static MyApplication *_instance;

	bool _shuttingDown;

	bool _parseLoaded;
	bool _resourceLoaded;
	void EndLoad();
};

namespace GameLua
{
	bool isWindows();
	bool FBIsLoggedIn();
	// если установить данную IPlaybackOperation на FlashMovieClip, то он проиграется только один раз
	IPlaybackOperation* getPlayOnceOperation();
}

#endif //APP_H_INCLUDED