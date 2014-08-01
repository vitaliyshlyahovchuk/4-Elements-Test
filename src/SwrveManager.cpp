#include "stdafx.h"
#include "SwrveManager.h"
#include "GameInfo.h"
#include "LevelInfoManager.h"

const std::string SwrveManager::ALL_CURRENCY = "AllCurrency";
const std::string SwrveManager::REAL_CURRENCY = "RealCurrency";
const std::string SwrveManager::LIVE_CURRENCY = "Lives";

#ifdef ENGINE_TARGET_WIN32

void SwrveManager::TrackEvent(const std::string& event_name, const std::string& payload) {
	Log::Debug("Swrve:TrackEvent:[" + event_name + "," + payload + "]");
}
void SwrveManager::TrackPurchase(const std::string& item, int cost, const std::string& currency, int quantity) {}
void SwrveManager::Start() {}
void SwrveManager::TrackBuyCurrency(const std::string &currencyPackId) {}
void SwrveManager::TrackCurrencyGiven(const std::string &currency, int amount) {}
void SwrveManager::OnAppEnterBackground() {}
void SwrveManager::OnAppBecomeActive() {}
void SwrveManager::OnAppTerminate() {}

#else

#include "swrve_wrapper.h"
#include "MyEngineWrapper.h"
#include "MicroTransactions.h"

#ifdef ENGINE_TARGET_IPHONE

@interface SwrveTimer : NSObject
- (void)onTimer: (NSTimer *) timer;
@end

@implementation SwrveTimer
- (void) onTimer: (NSTimer *) timer
{
	SwrveWrapper::SendQueuedEvents();
}
@end

static NSTimer *swrveTimer = nil;
static SwrveTimer *swrveTimerDelegate = nil;

#else

#include <jni.h>
#include "android/android.h"
#include "android/jni_support.h"

using namespace Android;

#endif

static int GetTimeInterval()
{
#if defined(DEBUG) || defined(_DEBUG)
	return 60;
#else
	return gameInfo.getConstInt("SwrveGetResourcesInterval", 6*60*60);
#endif
}

static void CheckResources()
{
	if (SwrveWrapper::IsSessionActive()) return;

	// Запрашиваем ресурсы из swrve.
	if (CheckTimeInterval("SwrveTime", GetTimeInterval()) ) {
		SwrveWrapper::DownloadUserResources();
	}
}

static void OnSessionStarted()
{
	CheckResources();
}

std::map<std::string, std::string> OnGetUserInfo()
{
	std::map<std::string, std::string> res;
	if (!SwrveWrapper::IsSessionActive()) return res;

	// level
	res["level"] = utils::lexical_cast(gameInfo.getLocalInt("extreme_level"));

	// location
#ifdef ENGINE_TARGET_IPHONE
	NSString * countryCode = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];
	res["location"] = [countryCode UTF8String];
#else
	JNIEnv *env = getEnv();
	JniClass clazz(env, "com/playrix/lib/Playrix");
	jmethodID method = env->GetStaticMethodID(clazz, "getLocaleCountry", "()Ljava/lang/String;");
	JniString jstr(env, (jstring)env->CallStaticObjectMethod(clazz, method));
	res["location"] = jstr.str();
#endif

	// install date
	if (gameInfo.getLocalString("InstallDate", "") == "") {
#ifdef ENGINE_TARGET_IPHONE
		NSDate *date = [NSDate date];
		NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
		[dateFormatter setTimeStyle:NSDateFormatterNoStyle];
		[dateFormatter setDateStyle:NSDateFormatterMediumStyle];
		[dateFormatter setDateFormat:@"yyyyMMdd"];
		std::string sDate = [[dateFormatter stringFromDate:date] UTF8String];
		gameInfo.setLocalString("InstallDate", sDate);
#else
		JniClass utilsclass(env, "com/playrix/fourelementsfreemium/CommonUtils");
		jmethodID get = env->GetStaticMethodID(utilsclass, "getFormattedDate", "()Ljava/lang/String;");
		JniString jdate(env, (jstring)env->CallStaticObjectMethod(utilsclass, get));
		gameInfo.setLocalString("InstallDate", jdate.str());
#endif
	}
	res["install"] = gameInfo.getLocalString("InstallDate");

	// TODO Запоминаем отправленные значения, чтобы не отправлять их, если они не меняются.

	return res;
}

static void OnResourcesDownloaded(SwrveWrapper::DownloadStatus::Type status, const std::string &result)
{
	SaveTimestamp("SwrveTime", GetTimestamp()); // Запоминаем, когда последний раз обновляли ресурсы, чтобы не делать это слишком часто.

	using namespace SwrveWrapper;
	if (status == DownloadStatus::SUCCEEDED) {
		// Обрабатываем данные.
		levelsInfo.OnABInfo(result.c_str());
	}
	else if (status == DownloadStatus::FAILED) {
		Log::Info("Swrve download resources failed.");
	}
	else {
		Log::Info("Swrve download resources - user changed.");
	}
}

static std::string GetUserId()
{
	std::string gameId = gameInfo.getLocalString("gameId");
	if (gameId.empty())
	{
		gameId = gameInfo.GetUUID(8);
		gameInfo.setLocalString("gameId", gameId);
	}
	return gameId;
}

void SwrveInit()
{
	int appId = 0;
	std::string apiKey;
	int timeInterval = gameInfo.getConstInt("SwrveTimerInterval", 5 * 60);
#ifdef ENGINE_TARGET_IPHONE
	NSBundle* bundle = [NSBundle mainBundle];
	appId = [[bundle objectForInfoDictionaryKey:@"SwrveGameId"] integerValue];
	NSString *apiKeyStr = [bundle objectForInfoDictionaryKey:@"SwrveApiKey"];
	apiKey = [apiKeyStr UTF8String];

	swrveTimerDelegate = [[[SwrveTimer alloc] init] autorelease];
	swrveTimer = [NSTimer scheduledTimerWithTimeInterval:timeInterval
																 target:swrveTimerDelegate selector:@selector(onTimer:) userInfo:nil repeats:YES];
#else
		JNIEnv *env = getEnv();
		JniClass clazz(env, "com/playrix/fourelementsfreemium/CommonUtils");
		jmethodID method = env->GetStaticMethodID(clazz, "startSwrveSendTimer", "(I)V");
		env->CallStaticVoidMethod(clazz, method, timeInterval);
#endif
	// для андроида id и ключ здесь неважны, это задается в яве
	SwrveWrapper::Init(appId, apiKey, OnGetUserInfo, OnResourcesDownloaded, OnSessionStarted, GetUserId);
}

bool SwrveManager::enableLogging() {
	//return gameInfo.getLocalInt("user_type", 0) == 0;
#ifdef ENTERPRISE
	return true;
#else
	return false;
#endif
}

void SwrveManager::Start()
{
	if (isJailbroken()) { return; }

	static bool needInit = true;
	if (needInit) {
		needInit = false;
		SwrveInit();
	}
	if (enableLogging()) {
		SwrveWrapper::StartSession();
	} else if (SwrveWrapper::IsSessionActive()) {
		SwrveWrapper::StopSession();
	}
}

void SwrveManager::TrackEvent(const std::string &event_name, const std::string& payload)
{
	if (event_name.empty()) { Assert(false); return; }
	if (!enableLogging()) return;
	SwrveWrapper::TrackEvent(event_name, payload);
}

void SwrveManager::TrackBuyCurrency(const std::string &productId)
{
	Assert(false);
}

void SwrveManager::TrackPurchase(const std::string& item, int cost, const std::string& ctype, int quantity)
{
	if (!enableLogging()) return;
	SwrveWrapper::TrackPurchase(item, cost, ctype, quantity);
}

void SwrveManager::TrackCurrencyGiven(const std::string &currency, int amount)
{
	if (amount <= 0) { assert(false); return; }
	if (!enableLogging()) return;
	SwrveWrapper::TrackCurrencyGiven(currency, amount);
}

#ifdef ENGINE_TARGET_IPHONE

void SwrveManager::OnAppEnterBackground()
{
	if (!enableLogging()) return;
	SwrveWrapper::OnApplicationDidEnterBackground();
}

void SwrveManager::OnAppBecomeActive()
{
	if (!enableLogging()) return;
	SwrveWrapper::OnApplicationDidBecomeActive();
}

void SwrveManager::OnAppTerminate()
{
	if (swrveTimer) {
		[swrveTimer invalidate];
		swrveTimer = nil;
	}
	if (!enableLogging()) return;
	SwrveWrapper::OnApplicationWillTerminate();
}

#endif

#endif // IOS + Android
