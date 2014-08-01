#include "stdafx.h"
#include "MyApplication.h"
#include "GameInfo.h"
#include "GameScriptMap.h"
#include "DownloadPackage.h"


#ifdef ENGINE_TARGET_ANDROID
	#include "android/android.h"
#endif

#ifdef ENGINE_TARGET_IPHONE
	#include "MM/AudioSession.h"
#endif

MyApplication *MyApplication::_instance = NULL;
int MyApplication::GAME_HEIGHT = 0;
int MyApplication::GAME_WIDTH = 0;

bool inPreloadResource = false;

class MyLoadScreen
	: public Core::LoadScreen
{
public:
	MyLoadScreen() : prev_time(0.0) { }

	void Init() {
	}

	void Update() {
		if (inPreloadResource) {
			gameInfo.Update(0.f);
			prev_time = gameInfo.getGlobalTime() - 0.02 - 0.0001;
			return;
		}
		gameInfo.Update(0.f);
		double time = gameInfo.getGlobalTime();
		double dt = time - prev_time;
		if (dt > 0.02) {
			prev_time = time;
			Core::mainScreen.Update(static_cast<float>(dt));
			Core::appInstance->Draw();
		}
	}

private:
	double prev_time;
};

#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
MyApplication::MyApplication()
	: Core::Application(new MyLoadScreen())
	, _parseLoaded(false)
#elif defined(ENGINE_TARGET_WIN32)
MyApplication::MyApplication(HINSTANCE hInstance, int nCmdShow, bool fullscreenMode)
	: Core::Application(hInstance, nCmdShow, fullscreenMode, new MyLoadScreen())
	, _parseLoaded(true)
#else
MyApplication::MyApplication()
	: Core::Application(new MyLoadScreen())
	, _parseLoaded(true)
#endif
	, _resourceLoaded(false)
	, _shuttingDown(false)
{
#ifndef PRODUCTION
	showFps = true;
#endif
	Assert(_instance == NULL);
	_instance = this;
}

MyApplication::~MyApplication() {
	if( loadScreen){
		delete loadScreen;
		loadScreen = NULL;
	}
}

void MyApplication::PreloadResources() {
	inPreloadResource = true;
	Core::LuaExecuteStartupScript("scripts/preload_start.lua");
	inPreloadResource = false;
}

void MyApplication::LoadResources() {
#ifdef ENGINE_TARGET_WIN32
	RealLoadResources();
#endif
#ifdef ENGINE_TARGET_ANDROID
	// TODO temp
	RealLoadResources();
#endif
}

void MyApplication::RealLoadResources() {
	Core::LuaExecuteStartupScript("scripts/start.lua");
	_resourceLoaded = true;
#ifdef ENGINE_TARGET_ANDROID
	//!! TODO parse
	_parseLoaded = true;
	Core::guiManager.getLayer("LoadScreen")->AcceptMessage(Message("Update Done"));
#endif
	if (_parseLoaded) {
		EndLoad();
	}
}

void MyApplication::SetParseLoaded() {
	_parseLoaded = true;
	if (_resourceLoaded) {
		EndLoad();
	}
}

void MyApplication::EndLoad() {
	Core::LuaCallVoidFunction("onApplicationEndLoad");
}

void MyApplication::ScriptMap() {
	Core::Application::ScriptMap();
	gameScriptMap();
}

void MyApplication::RegisterTypes() {
	Core::Application::RegisterTypes();
	gameRegisterTypes();
}

void MyApplication::DrawPause() {
	// ничего не рисуем
}

void MyApplication::PostDraw() {
}

#ifdef ENGINE_TARGET_WIN32
void MyApplication::DrawFps()
{
	if (gameInfo.IsDevMode() && Render::isFontLoaded("debug") ) {
		Render::BindFont("debug");
		Render::device.PushMatrix();
		Render::device.MatrixTranslate( math::Vector3(Render::device.Width() - 35.f, 0.f, 0.f));
		Render::PrintString(0.f, 20.f, std::string("fps ")+utils::lexical_cast(currentFps), 1.0f, RightAlign);
		Render::PrintString(0.f, 40.f, std::string("mem ")+utils::lexical_cast(Render::device.GetVideoMemUsage()), 1.0f, RightAlign);
		Render::device.PopMatrix();
	}
}
#else
void MyApplication::DebugDraw()
{
	if (showFps && Render::isFontLoaded("debug"))
	{
		Render::BindFont("debug");
		int x = Render::device.Width() - 5;
		Render::PrintString(x, 60, std::string("fps ")+utils::lexical_cast(currentFps), 1.0f, RightAlign);
		Render::PrintString(x, 40, std::string("V ")+utils::lexical_cast(Render::device.GetVideoMemUsage()/1024)+std::string("k"), 1.0f, RightAlign);
		Render::PrintString(x, 20, std::string("M ")+utils::lexical_cast(utils::getProcSize()/1024)+std::string("k"), 1.f, RightAlign);
		if(debugString.size()) {
			Render::PrintString(x, 80, debugString, 1.0f, RightAlign);
		}
	}
}
void MyApplication::BaseDraw() {
	Core::mainScreen.Draw();
	PostDraw();
}
#endif

MyApplication *MyApplication::GetInstance() {
	Assert(_instance != NULL);
	return _instance;
}

void MyApplication::MarkAsShuttingDown()
{
	_shuttingDown = true;
}

bool MyApplication::IsShuttingDown() const
{
	return _shuttingDown;
}

void MyApplication::Update(float dt)
{
	gameInfo.Update(dt);
	Application::Update(dt);
}

void MyApplication::PreInit(const std::string &workDir)
{
	Core::fileSystem.SetWriteDirectory(File::GetSpecialFolderPath(SpecialFolder::LocalData));

	std::string settings_path = workDir + "Settings.xml";
#if defined(ENGINE_TARGET_WIN32)
	if(File::ExistsInFs(workDir + "Settings_pers.xml")) {
		settings_path = workDir + "Settings_pers.xml";
	}
#elif defined(ENGINE_TARGET_ANDROID)
	// android
	Core::fileSystem.MountZip("/sdcard/assets.zip", "assets");
//	Core::fileSystem.MountZip(Android::getAppPath(), "assets");
	std::vector<std::pair<std::string, IO::FileInfo::Type> > searchPaths;
	searchPaths.push_back(std::make_pair("base", IO::FileInfo::TYPE_NORMAL));
	Core::fileSystem.GenIndexMap(searchPaths);
#endif

	Xml::RapidXmlDocument doc(settings_path);
	rapidxml::xml_node<>* root = doc.first_node();
	
#ifndef ENGINE_TARGET_ANDROID
	std::string base_path = workDir + Xml::GetStringAttributeOrDef(root, "path", "");
	Core::fileSystem.MountDirectory(base_path);
	File::cd(base_path);
#endif
	
#if defined(ENGINE_TARGETY_WIN32)
	//win
	Core::locale.SetSupportedLanguages("ru;en");
	
#elif defined(ENGINE_TARGET_IPHONE)
	// ios
	Core::fileSystem.MountDirectory(".");
	Core::fileSystem.MountDirectory(PackageUpdater::WORK_PATH);
	
#endif

	Log::log.AddSink(new Log::TextFileLogSink("log.txt", true));

#ifdef _DEBUG
	Log::log.SetMinSeverityLevel(Log::Severity::Debug);
#else
	Log::log.SetMinSeverityLevel(Log::Severity::Info);
#endif

	gameInfo.Init();
	gamePreInit(root);
}

void MyApplication::Release()
{
	Assert2(luabind::type(luabind::globals(Core::luaState)["onApplicationRelease"]) == LUA_TFUNCTION, "Нужно определить функцию: \"onApplicationRelease()\"");
	Core::LuaCallVoidFunction("onApplicationRelease");
	gameInfo.Save(false);
}

void MyApplication::InitAudioSession()
{
	//Инициализировать аудиосессию нужно сразу после инициализации приложения
	//, а лучше вообще в Application::Init() там где MM::manager.Init()
#ifdef ENGINE_TARGET_IPHONE
	MM::AudioSession::ChooseAppropriateCategory();
	MM::AudioSession::Init();
#endif
}

bool MyApplication::IsOtherAudioPlaying()
{
#ifdef ENGINE_TARGET_IPHONE
	return MM::AudioSession::IsOtherAudioPlaying();
#else
	return false;
#endif
}
