#include "stdafx.h"
#include "MyApplication.h"
#include "GameInfo.h"
#include "FBInterface.h"
#include "SwrveManager.h"
#include "ParseManager.h"
#include "FriendInfo.h"
#include "MicroTransactions.h"
#include "AppSettings.h"
#include "DownloadPackage.h"
#include "InternetImageDownLoader.h"

#ifdef ENGINE_TARGET_IPHONE

#import <UIKit/UIKit.h>
#import "Reachability.h"
#include "MyEngineWrapper.h"

#else

#include <jni.h>
#include "android/android.h"
#include "android/jni_support.h"
#include "temp.h"

using namespace Android;

#endif

boost::intrusive_ptr<InternetImageDownLoader> imgLoader;

MyApplication* application = NULL;

FB::Parameters lastRequestParams;
bool fbSilentLogOut = false;

void appRequestLoaded();
void loadSyncFriendInfo();
void saveCachedFrinedInfo();
bool loadCachedFriendInfo();
void initAskHelpDialog();

void CheckNetworkStatus()
{
	std::string nw = "";
#ifdef ENGINE_TARGET_IPHONE
	NetworkStatus network_status = [[Reachability reachabilityForInternetConnection] currentReachabilityStatus];
	switch(network_status) {
		case NotReachable: nw = "No"; break;
		case ReachableViaWiFi: nw = "WiFi"; break;
		case ReachableViaWWAN: nw = "WWAN"; break;
		default: nw = std::string("Type") + utils::lexical_cast(network_status); break;
	}
#else
	JNIEnv *env = getEnv();
	JniClass clazz(env, "com/playrix/lib/Playrix");
	jmethodID method = env->GetStaticMethodID(clazz, "getInternetConnectionType", "()Ljava/lang/String;");
	JniString jstr(env, (jstring)env->CallStaticObjectMethod(clazz, method));
	nw = jstr.str();
#endif
	SwrveManager::TrackEvent("UserInfo.InternetConnection", "{\"Type\":\"" + nw + "\"}");
}

// возвращает релизную версию приложения из трех цифр, аля 1.0.2
// которая обновляется нечасто, в идеале только с каждым новым апдейтом для аппстора
std::string GetAppVersion()
{
#ifdef ENGINE_TARGET_IPHONE
	NSString * bundle_ver = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	return [bundle_ver UTF8String];
#else
	JNIEnv *env = getEnv();
	JniClass clazz(env, "com/playrix/lib/Playrix");
	jmethodID method = env->GetStaticMethodID(clazz, "getAppVersion", "()Ljava/lang/String;");
	JniString jstr(env, (jstring)env->CallStaticObjectMethod(clazz, method));
	
	return jstr.str();
#endif
}

// возвращает версию/номер билда из одного числа, которое увеличивается с каждой сборкой, внутренней или релизной
std::string GetBuildNumberStr()
{
#ifdef ENGINE_TARGET_IPHONE
	NSString * bundle_ver = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	return [bundle_ver UTF8String];
#else
	JNIEnv *env = getEnv();
	JniClass clazz(env, "com/playrix/lib/Playrix");
	jmethodID method = env->GetStaticMethodID(clazz, "getAppVersionCode", "()Ljava/lang/String;");
	JniString jstr(env, (jstring)env->CallStaticObjectMethod(clazz, method));
	
	return jstr.str();
#endif
}

int GetBuildNumber()
{
	return utils::lexical_cast<int>( GetBuildNumberStr() );
}

std::string Device_SystemVer()
{
#ifdef ENGINE_TARGET_IPHONE
	NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
	return [systemVersion UTF8String];
#else
	JNIEnv *env = getEnv();
	JniClass clazz(env, "com/playrix/lib/Playrix");
	jmethodID method = env->GetStaticMethodID(clazz, "getOsVer", "()Ljava/lang/String;");
	JniString jstr(env, (jstring)env->CallStaticObjectMethod(clazz, method));

	return jstr.str();
#endif
}

std::string Device_SystemLang()
{
#ifdef ENGINE_TARGET_IPHONE
	NSString* deviceLanguage = [[NSLocale preferredLanguages] objectAtIndex:0];
	return [deviceLanguage UTF8String];
#else
	JNIEnv *env = getEnv();
	JniClass clazz(env, "com/playrix/lib/Playrix");
	jmethodID method = env->GetStaticMethodID(clazz, "getDeviceLanguage", "()Ljava/lang/String;");
	JniString jstr(env, (jstring)env->CallStaticObjectMethod(clazz, method));

	return jstr.str();
#endif
}

void OpenURL(const std::string& url)
{
	utils::OpenPath(url);
}

void OpenFacebookPage()
{
#ifdef ENGINE_TARGET_IPHONE
	// Сначала пытаемся открыть страницу игры в приложении facebook,
	std::string stdStringUrl = "fb://profile/" + Core::GlobalConstants::GetString("FacebookAppPageID");
	NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:stdStringUrl.c_str()]];
	
	UIApplication* app = [UIApplication sharedApplication];
	if (![app canOpenURL:url]) {
		// если не получается, то в браузере
		stdStringUrl = gameInfo.getConstString("FacebookGamePage");
		url = [NSURL URLWithString:[NSString stringWithUTF8String:stdStringUrl.c_str()]];
	}
	[app openURL:url];
#else
	// TODO
#endif
}

void addLikeCash()
{
	gameInfo.setLocalBool("IsMoneyAddedAfterLikeGame", true);
	SwrveManager::TrackPurchase("currency_given.Like", gameInfo.getConstInt("FBLikeAward"), SwrveManager::ALL_CURRENCY);
}

void FacebookSilentLogout() {
	fbSilentLogOut = true;
	FB::LogOut();
}

void facebookCallback(FB::Action::Type action)
{
	if (action == FB::Action::LOGIN_COMPLETED) {
		// логин есть, дождемся получения имени и идентификатора
		return;
	}
	if (action == FB::Action::USERINFO_AVAILABLE) {
		FB::RequestFriendsInformation();
		ParseManager::RegisterFacebookID(FB::UserId(), FB::UserFirstName());
		Core::LuaCallVoidFunction("onFacebookLogin");
		return;
	}
	if (action == FB::Action::FRIENDSINFO_AVAILABLE) {
		loadSyncFriendInfo();
		return;
	}
	if (action == FB::Action::LOGIN_FAILED) {
		Core::LuaCallVoidFunction("onFacebookLoginFailed");
		ParseManager::RegisterFacebookID("", "");
		return;
	}
	if (action == FB::Action::USERINFO_FAILED) {
		if (FB::IsLoggedIn()) {
			// сохранненная сессия есть, а к фейсбуку соедениться не можем, используем кэш друзей
			// если кэша нет, разлогиниваемся
			if (loadCachedFriendInfo()) {
				ParseManager::RegisterFacebookID(gameInfo.getLocalString("fbId"), gameInfo.getLocalString("fbName"));
				// фиктивно: сказать игре, что залогинились
				Core::LuaCallVoidFunction("onFacebookLogin");
				// фиктивно: сказать игре, что получили сохранения друзей
				Core::LuaCallVoidFunction("ParseFriendsLoaded");
			} else {
				Core::LuaCallVoidFunction("onFacebookLoginFailed");
				ParseManager::RegisterFacebookID("", "");
				FacebookSilentLogout();
			}
		} else {
			ParseManager::RegisterFacebookID("", "");
		}
		return;
	}
	if (action == FB::Action::APPREQUEST_AVAILABLE) {
		appRequestLoaded();
		return;
	}
	if (action == FB::Action::LOGOUT) {
		if (!fbSilentLogOut) {
			Core::LuaCallVoidFunction("onFacebookLogout");
			gameInfo.getAvatar()->SetTexture(NULL);
			for (size_t i = 0; i < gameInfo.friends.size(); ++i) {
				delete gameInfo.friends[i];
			}
			gameInfo.friends.clear();
			ParseManager::RegisterFacebookID("", "");
			return;
		}
		fbSilentLogOut = false;
		return;
	}
	if (action == FB::Action::REQUEST_SENT) {
		std::string helpType = lastRequestParams["data"];
		if (helpType != "") {
			std::vector<std::string> uids = utils::String::Split(lastRequestParams["to"], ',');
			double timeToNextRequest = gameInfo.getGlobalTime() + gameInfo.getConstDouble("RequestSpanTime", 3600.0);
			for (size_t i = 0; i < uids.size(); ++i) {
				gameInfo.setLocalDouble(helpType + "_" + uids[i], timeToNextRequest);
			}
			gameInfo.Save(false);
			Core::LuaCallVoidFunction("onAppRequestSent");
		}
		return;
	}
	if (action == FB::Action::REQUEST_FAILED) {
		Core::LuaCallVoidFunction("onAppRequestFailed");
		return;
	}
	if (action == FB::Action::LIKESINFO_AVAILABLE) {
		if (FB::IsGameLiked()) {
			if (!gameInfo.getLocalBool("IsMoneyAddedAfterLikeGame", false)) {
				gameInfo.addCash(gameInfo.getConstInt("FBLikeAward"), false, &addLikeCash);
				bool firtsTime = !gameInfo.getLocalBool("fbLikeInfoAvailable");
				SwrveManager::TrackEvent("LikeUsOnFacebook.Yes",
										 "{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("current_level")) +
										 ",\"FirstTime\":" + (firtsTime?"true":"false") +"}");
			}
			Core::LuaDoString("marketingActions:disableLikeGameAction()");
		} else {
			SwrveManager::TrackEvent("LikeUsOnFacebook.Finish", "{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("current_level")) + "}");
		}
		gameInfo.setLocalBool("tryLikeGamePage", false);
		gameInfo.setLocalBool("fbLikeInfoAvailable", true);
		gameInfo.Save(false);
		return;
	}
	if (action == FB::Action::LIKESINFO_FAILED) {
		gameInfo.setLocalBool("tryLikeGamePage", false);
		return;
	}
}

static std::string cachedFriendInfoFilePath;

static void InitCachesPath()
{
	if (cachedFriendInfoFilePath.empty()) {
#ifdef ENGINE_TARGET_IPHONE
		cachedFriendInfoFilePath = File::GetSpecialFolderPath(SpecialFolder::LocalCaches) + "/friends.xml";
#else
		cachedFriendInfoFilePath = "Caches/friends.xml";
#endif
	}
}

void saveCachedFrinedInfo()
{
	InitCachesPath();
	rapidxml::xml_document<char> doc;
	rapidxml::xml_node<char>* root = Xml::NewNode(&doc, "root");
	for (size_t i = 0; i < gameInfo.friends.size(); ++i) {
		FriendInfo* info = gameInfo.friends[i];
		rapidxml::xml_node<char>* f = Xml::NewNode(root, "friend");
		Xml::SetStringAttribute(f, "id", info->fb_id);
		Xml::SetStringAttribute(f, "name", info->name);
		Xml::SetIntAttribute(f, "currentMarker", info->current_marker);
		Xml::SetIntAttribute(f, "extreme_level", info->extreme_level);
		rapidxml::xml_node<char>* levels = Xml::NewNode(f, "levels");
		for (size_t i = 0; i < info->level_scores.size(); ++i) {
			rapidxml::xml_node<char>* level = Xml::NewNode(levels, "level");
			Xml::SetIntAttribute(level, "score", info->level_scores[i]);
		}
	}
	std::ofstream file;
	file.open(cachedFriendInfoFilePath.c_str());
	file << doc;
}

bool loadCachedFriendInfo()
{
	InitCachesPath();
	Assert(imgLoader.get());
	Assert(gameInfo.friends.size() == 0);
	if (Core::fileSystem.FileExists(cachedFriendInfoFilePath)) {
		//сначала попробуем загрузить аватарку самого игрока
		imgLoader->LoadFromCache(gameInfo.getAvatar(), gameInfo.getLocalString("fbId"));
		Xml::RapidXmlDocument doc(cachedFriendInfoFilePath);
		rapidxml::xml_node<>* root = doc.first_node();
		rapidxml::xml_node<>* f = root->first_node();
		while (f) {
			FriendInfo* info = new FriendInfo();
			info->fb_id = Xml::GetStringAttribute(f, "id");
			info->name = Xml::GetStringAttribute(f, "name");
			info->current_marker = Xml::GetIntAttribute(f, "currentMarker");
			info->extreme_level = Xml::GetIntAttribute(f, "extreme_level");
			rapidxml::xml_node<char>* level = f->first_node("levels")->first_node();
			while (level) {
				info->level_scores.push_back(Xml::GetIntAttribute(level, "score"));
				level = level->next_sibling();
			}
			gameInfo.friends.push_back(info);
			imgLoader->LoadFromCache(info->avatar, info->fb_id);
			f = f->next_sibling();
		}
		return true;
	}
	return false;
}

void loadSyncFriendInfo()
{
	if (FB::IsLoggedIn() && FB::IsWaitingForFriends()) {
		return; // ждем fb друзей
	}
	ParseManager::LoadFriendsInfo();
}

// поставить на загрузку изображения друзей, ... и изображение игрока, конечно тоже
void LoadUserPics()
{
	Assert(imgLoader.get());
	if (FB::IsLoggedIn()) {
		imgLoader->LoadFromInternet(gameInfo.getAvatar(), FB::UserPicUrl(), FB::UserId());
	}
	for (size_t i = 0; i < gameInfo.friends.size(); ++i) {
		bool hasPic = false;
		FriendInfo* info = gameInfo.friends[i];
		FB::Friend f = FB::GetFriend(info->fb_id);
		if (f.pic_url != "") {
			hasPic = true;
			imgLoader->LoadFromInternet(info->avatar, f.pic_url, info->fb_id);
		}
	}
}

// Парсе загрузил сохранку
void ParseEndLoad()
{
	application->SetParseLoaded();
}

// Парсе перегрузил сохранку
void ParseReload()
{
	Core::LuaCallVoidFunction("ReloadGameInfo");
}

void ParseFriendsLoaded()
{
	// сказать игре, что данные о друзьях загрузились
	Core::LuaCallVoidFunction("ParseFriendsLoaded");
	// запросим картинки друзей
	LoadUserPics();
	// запросим "запросы приложения"
	if (FB::IsLoggedIn()) {
		FB::RequestAppRequestsInformation();
	}
	// сохраним друзей в локальный кэш для запуска без интернета
	saveCachedFrinedInfo();
	// аватарки загружены, можно добавить друзей в askHelpDialog
	initAskHelpDialog();
}

void appRequestLoaded()
{
	Core::LuaCallVoidFunction("MessageCenterClearRequests");
	const std::vector<FB::AppRequest>& requests = FB::GetAppRequests();
	std::map<std::string, FriendInfo*> fb_friends; // фейсбучные друзья
	for (size_t i = 0; i < gameInfo.friends.size(); ++i) {
		fb_friends[gameInfo.friends[i]->fb_id] = gameInfo.friends[i];
	}

	// для поиска дубликатов:
	std::map<std::string, std::vector<std::string> > requestsMap; // map < "friendId", vector < "requestData" > >
	// реквесты, требующие ответные действий от игрока
	std::map<std::string, bool> needAnswerRequestData;
	needAnswerRequestData["live"] = true;
	needAnswerRequestData["need_live"] = true;
	needAnswerRequestData["ticket"] = true;
	needAnswerRequestData["need_ticket"] = true;
	// идентификаторы реквестов, которые нужно удалить
	std::vector<std::string> needDeleteRequests;

	for (std::vector<FB::AppRequest>::const_iterator i = requests.begin(), end = requests.end(); i != end; ++i) {
		if (fb_friends[i->senderId] && needAnswerRequestData[i->data]) { // в всписке друзей (парс может вернуть меньше друзей, чем на самом деле)
			std::vector<std::string> &requestTypes = requestsMap[i->senderId];
			if (std::find(requestTypes.begin(), requestTypes.end(), i->data) != requestTypes.end()) {
				// дублирующийся реквест
				needDeleteRequests.push_back(i->requestId);
			} else {
				// такого реквеста еще нет
				requestTypes.push_back(i->data);
				FriendInfo *info = fb_friends[i->senderId];
				Core::LuaCallVoidFunction("MessageCenterAddRequest", info, i->data, i->requestId);
			}
		} else {
			needDeleteRequests.push_back(i->requestId); // удалим
		}
	}
	// в движке пока нет массового удаления реквестов, удаляем по одному
	for (size_t i = 0; i < needDeleteRequests.size(); ++i) {
		FB::DeleteAppRequest(needDeleteRequests[i]);
	}
}

void initAskHelpDialog()
{
	Core::LuaCallVoidFunction("AskHelpClearFriends");
	for (size_t i = 0; i < gameInfo.friends.size(); ++i) {
		FriendInfo *info = gameInfo.friends[i];
		Core::LuaCallVoidFunction("AskHelpAddFriend", info->fb_id , info->name, info);
	}
}

// Сделать запрос = идентификаторы друзей через запятую + тип помощи
void PostAppRequests(const std::string& friendUids, const std::string& helpType)
{
	lastRequestParams.clear();
	lastRequestParams["data"] = helpType;
	lastRequestParams["to"] = friendUids;
	std::string message = Core::resourceManager.Get<Render::Text>("fbRequest_" + helpType)->ToString();
	lastRequestParams["message"] = utils::String::Replace(message, "<user>", gameInfo.getLocalString("fbName"));
	FB::SendRequest(lastRequestParams);
}

void InviteFriends()
{
	lastRequestParams.clear();
	lastRequestParams["title"] = Core::resourceManager.Get<Render::Text>("FBInviteTitle")->GetSource();
	lastRequestParams["message"] = Core::resourceManager.Get<Render::Text>("FBInviteMessage")->GetSource();
	FB::SendRequest(lastRequestParams);
}

void appMakePurchase(const std::string& purchaseId)
{
#ifdef ENGINE_TARGET_IPHONE
	InApp::MakePurchase(Core::GlobalConstants::GetString("purchase_prefix") + purchaseId, nil, 1);
#else
	InApp::MakePurchase(Core::GlobalConstants::GetString("purchase_prefix") + purchaseId, "", 1);
#endif
}

void appPurchaseComplete(std::string purchaseId)
{
	std::string purchase_prefix = Core::GlobalConstants::GetString("purchase_prefix");
	if (utils::String::StartsWith(purchaseId, purchase_prefix)) {
		purchaseId = purchaseId.substr(purchase_prefix.size());
	}
	gameInfo.buy(purchaseId);
	Core::LuaCallVoidFunction("onPurchaseComplete", purchaseId);
	gameInfo.setLocalBool("payer", true);
}

void appPurchaseFail(std::string purchaseId)
{
	std::string purchase_prefix = Core::GlobalConstants::GetString("purchase_prefix");
	if (utils::String::StartsWith(purchaseId, purchase_prefix)) {
		purchaseId = purchaseId.substr(purchase_prefix.size());
	}
	Core::LuaCallVoidFunction("onPurchaseFail", purchaseId);
}

void ReportIssue()
{
	std::string message("@");
	message += "<p>" + Core::resourceManager.Get<Render::Text>("ReportAnIssueLetterBody")->ToString() + "</p>";
	message += "<br/>";
	
	std::string time;
	{
		std::time_t rawtime = std::time(0);
		std::tm* timeinfo = std::localtime(&rawtime);
		char buffer[128];
		strftime(buffer, 128, "%F %R %z", timeinfo);
		time.assign(buffer);
	}
	
	std::string sysVer = Device_SystemVer();
	std::string deviceLang = Device_SystemLang();
	
	message += "<p>";
	message += Core::resourceManager.Get<Render::Text>("ReportAnIssueLetterBody2")->ToString() + "<br>";
	message += "Game: " + gameInfo.getConstString("GameName") + "<br>";
	message += "Time: " + time + "<br>";
	message += "Build time: " + std::string(__DATE__) + " " + std::string(__TIME__) + "<br/>";
	message += "Version: " + GetAppVersion() + "." + GetBuildNumberStr() + "<br>";
	message += "Language: " + Core::locale.GetLanguage() + " (" + deviceLang + ")" + "<br>";
	message += "OS: " + sysVer + "<br>";
	message += "Device: " + DeviceDescription() + " [" + GetUniqueDeviceIdentifier() + "] <br>";
	message += std::string("-----") + "<br>";
	message += "<p>";
	
	Marketing::EmailDialog dialog;
	dialog.SetSubject("ReportAnIssueLetterSubject");
	dialog.SetMessageBody(message, Marketing::EmailDialog::MessageType::HTML, Marketing::EmailDialog::Extra::NO_INFO);
	dialog.AddRecipient("support@playrix.com");

	dialog.Show();
}

void deviceForSwrve(std::string &type, std::string &subtype)
{
#ifdef ENGINE_TARGET_IPHONE
	type = [[[UIDevice currentDevice] model] UTF8String];
#else
	type = "android";
#endif
	subtype = DeviceDescription();
}

namespace Playrix
{
	int screenWidth = 0;
	int screenHeight = 0;
}

IPoint GetResolutionSize(IPoint game_size)
{
#ifdef ENGINE_TARGET_IPHONE
	UIScreen* mainscr = [UIScreen mainScreen];
	IPoint size(mainscr.currentMode.size.width, mainscr.currentMode.size.height);
	//IPoint size2(mainscr.bounds.size.width, mainscr.bounds.size.height);
#else
	IPoint size(Playrix::screenWidth, Playrix::screenHeight);
#endif
	if(size.x > size.y)
	{
		std::swap(size.x, size.y);
	}
	float proportions = (size.x + 0.f)/size.y;
	if(proportions > 0.74)
	{
		//iPad
		return IPoint(768, 1024);
	}
	if(proportions > 0.66)
	{
		//iPhone
		return IPoint(640, 960);
	}
	if(proportions > 0.56)
	{
		//iPhone 5
		return IPoint(640, 1136);
	}
	return IPoint(640, int(640/proportions));
}

std::string GetDeviceLanguage()
{
	return Device_SystemLang().substr(0,2);
}

void ratingControlCallback(Marketing::RatingControl::Action::Type action)
{
	if (action == Marketing::RatingControl::Action::REJECTED ||
		action == Marketing::RatingControl::Action::POSTPONED)
	{
		// покажем панель старта уровня
		gameInfo.setLocalBool("NeedShowStartLevelPanel", true);
		Core::LuaCallVoidFunction("OnCardEventFinish");
	}
	if (action != Marketing::RatingControl::Action::SHOWN) {
		bool firstTime = gameInfo.getLocalInt("extreme_level") == 10;
		std::string event = firstTime ? "WindowFirstTime" : "Later";
		std::string choice = "";
		switch (action) {
			case Marketing::RatingControl::Action::REJECTED:
				choice = "No";
				SwrveManager::TrackEvent("Rating.Never", "{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("current_level")) + "}");
				break;
			case Marketing::RatingControl::Action::POSTPONED:
				choice = "Later";
				break;
			case Marketing::RatingControl::Action::ACCEPTED:
				choice = "Yes";
				SwrveManager::TrackEvent("Rating.Yes",
					std::string("{\"FirstTime\":") + ( firstTime ? "true" : "false") +
					",\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("current_level")) +
					"}");
				break;
			default:
				Assert(false);
				break;
		}
		SwrveManager::TrackEvent("Rating." + event,
			"{\"Choice\":\"" + choice + "\"" +
			",\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("current_level")) +
			"}");
	}
}

void UpdateGameInfoHash(std::string data)
{
#ifdef ENGINE_TARGET_IPHONE
	data = "SaltPrefix" + data + "SaltSuffix";
	NSString* hash = [EngineWrapper MD5FromString:[NSString stringWithUTF8String:data.c_str()]];
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	@synchronized(defaults) {
		[defaults setObject:hash forKey:@"GameInfoHash"];
		[defaults synchronize];
	}
#else
	// TODO
#endif
}

bool hasModificationGameInfo(std::string data)
{
#ifdef ENGINE_TARGET_IPHONE
	data = "SaltPrefix" + data + "SaltSuffix";
	NSString* hash = [EngineWrapper MD5FromString:[NSString stringWithUTF8String:data.c_str()]];
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	bool result = false;
	@synchronized(defaults) {
		NSString* saved_hash = [defaults objectForKey:@"GameInfoHash"];
		result = saved_hash != nil && ![hash isEqualToString: saved_hash];
	}
	return result;
#else
	// TODO
	return false;
#endif
}

void OnAppLaunched()
{
	imgLoader = new InternetImageDownLoader;

#if defined(PRODUCTION)
	Assert2(false, "Insert correct settings")
	ParseManager::Init("4uirKSuqIEX9a6CY03JrHG1BMCTwXIKZzup9uuQR", "EA9fKdrl157DaBeEem7uwVpvZKaKTx1U0raoekbc");
#elif defined(ENTERPRISE)
	ParseManager::Init("4uirKSuqIEX9a6CY03JrHG1BMCTwXIKZzup9uuQR", "EA9fKdrl157DaBeEem7uwVpvZKaKTx1U0raoekbc");
#else
	ParseManager::Init("eIidFIorzsyl2AGC3xumQVTdBAE2MpSRavIX2sii", "9PDKqm9Y53RDm0ShxDcQBjt6NBEFQf6JJLs4cXnb");
#endif

#ifdef ENGINE_TARGET_IPHONE
	std::string cachesPath = File::GetSpecialFolderPath(SpecialFolder::LocalCaches);
#else
	std::string cachesPath = "Caches/";
#endif
	// кеш, апдейты из интернета:
	PackageUpdater::WORK_PATH = cachesPath + "/update/current/";
	PackageUpdater::TEMP_PATH = cachesPath + "/update/load/";

#ifdef ENGINE_TARGET_IPHONE
	MyApplication::PreInit(std::string([[[NSBundle mainBundle] bundlePath] UTF8String]) + "/");
#else
	MyApplication::PreInit("");
#endif
	MyApplication::InitAudioSession();
	
	gameInfo.LoadFromFile();
	
	SwrveManager::Start();
	// On each session
	CheckNetworkStatus();
	
	::application = new MyApplication();
	::application->maxFps = 30;
#ifdef ENGINE_TARGET_IPHONE
	UIViewController* rootController = utils::getAppViewController();
	::application->Init((CAEAGLLayer*)rootController.view.layer, MyApplication::GAME_WIDTH, MyApplication::GAME_HEIGHT);
#else
	Core::appInstance->Init(MyApplication::GAME_WIDTH, MyApplication::GAME_HEIGHT);
#endif
	::application->Start();
	//Не знаю куда еще запихать инициализацию is_retina для gameInfo.
	gameInfo.setGlobalBool("is_retina", Render::device.ContentScaleFactor() == 2);
#ifdef ENGINE_TARGET_IPHONE
	::application->MainLoop();
#endif

	AppSettings::CheckReset();

	ParseManager::StartLoad();

	FB::InitWithCallback(&facebookCallback);

	math::random_seed((size_t)Core::Timer::getTime());
	Marketing::RatingControl::Init(0.f, 0.f, "", &ratingControlCallback);
}

void OnAppTerminated()
{
	imgLoader = NULL;
	((MyApplication*)::application)->Release();
	::application->ShutDown();
}

void OnAppActivated()
{
	if (gameInfo.getLocalBool("tryLikeGamePage")) {
		FB::RequestLikesInformation();
	}
	
	gameInfo.updateLocalNotifications();
	
	if (Core::mainScreen.isLayerOnScreen("GameLayer")) {
		Core::guiManager.getLayer("GameLayer")->getWidget("GameField")->AcceptMessage(Message("Continue"));
	}
	Core::LuaCallVoidFunction("applicationDidBecomeActive");
}

void OnAppDeactivated()
{
	math::random_seed((size_t)Core::Timer::getTime());
	gameInfo.Save(false);
	
	if (Core::mainScreen.isLayerOnScreen("GameLayer")) {
		Core::guiManager.getLayer("GameLayer")->getWidget("GameField")->AcceptMessage(Message("Pause"));
	}
}

void OnAppShown()
{
}

void OnAppCollapsed()
{
}
