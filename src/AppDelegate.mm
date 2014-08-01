#import "AppDelegate.h"
#import "EAGLView.h"
#import "Engine.h"
#import "HelpViewController.h"
#import "MicroTransactions.h"

#import "PlayrixEngine.h"
#import "MyEngineWrapper.h"
#import "GameInfo.h"
#import "LevelInfoManager.h"
#import "InternetImageDownLoader.h"
#import "DownloadPackage.h"
#import "Reachability.h"

#import <FacebookSDK/FacebookSDK.h>

#include "MyApplication.h"
#include "ParseManager.h"
#include "SwrveManager.h"
#include "FBInterface.h"
#include "FriendInfo.h"
#include "Utils/Str.h"
#include "Marketing/EmailDialog.h"
#include "GameUpdateChecker.h"
#include "FileChecker.h"
#include "AppSettings.h"

#include "Marketing/RatingControl.h"
#include "Marketing/LocalNotifications.h"
#include "MM/AudioSession.h"
#include "AppCommon.h"

extern MyApplication* application;
static DownloadPackage* packageDownloader = nil;

// fwd declare
void appPurchaseComplete(std::string purchaseId);
void appPurchaseFail(std::string purchaseId);

@interface AppDelegate()
// Declare private methods here
@end

@implementation AppDelegate

@synthesize window;
@synthesize glView;
@synthesize viewController;

extern bool ShellOnScreen;
extern int show_interval;

AppDelegate* instance = nil;
NSOperationQueue * queue = 0;
HelpViewController * helpViewController = nil;

void DownloadUpdateStart()
{
	NSLog(@"Checking for updates...");
}

void DownloadUpdateFinishCallback()
{
	NSLog(@"Checking for updates done");

	GameUpdateChecker::check();
	FileChecker::check();

	levelsInfo.LoadLevelMap();

	Core::guiManager.getLayer("LoadScreen")->AcceptMessage(Message("Update Done"));
}

void DownloadUpdateFinish(bool updated)
{
	application->RunInMainThread(DownloadUpdateFinishCallback);
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *) __unused application
{
	Core::LuaCollectGarbage();
	Flash::FlashResourceManager::instance->releaseMemory(true);
}

+ (AppDelegate*) GetInstance {
	return (AppDelegate*)[[UIApplication sharedApplication] delegate];
}

Core::Application* CreateApplication() {
	return NULL;
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (NSUInteger)application: (UIApplication *)application supportedInterfaceOrientationsForWindow: (UIWindow *)__unused window {
	return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
}
#endif

- (void) onPurchaseComplete:(NSNotification *)notification {
	appPurchaseComplete([notification.userInfo[@"productId"] UTF8String]);
}

- (void) onPurchaseFail:(NSNotification *)notification {
	appPurchaseFail([notification.userInfo[@"productId"] UTF8String]);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	NSBundle* bundle = [NSBundle mainBundle];

	Assert(instance == nil);
	instance = self;
	
	helpViewController = [[HelpViewController alloc] initWithAppDelegate:self];

	packageDownloader = [[DownloadPackage alloc] init];
	[packageDownloader setStartCallback:DownloadUpdateStart];
	[packageDownloader setFinishCallback:DownloadUpdateFinish];

	[[UIApplication sharedApplication] setStatusBarHidden:YES];
	if ([viewController respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
		// Hide status bar in iOS 7
		[viewController prefersStatusBarHidden];
		[viewController performSelector:@selector(setNeedsStatusBarAppearanceUpdate)];
	}

	NSNotificationCenter * center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(onPurchaseComplete:) name:@"MicroTransactionPurchaseFinished" object:nil];
	[center addObserver:self selector:@selector(onPurchaseFail:) name:@"MicroTransactionCancelled" object:nil];
	[center addObserver:self selector:@selector(onPurchaseFail:) name:@"MicroTransactionFail" object:nil];

#ifdef ENTERPRISE
	// Hockey SDK init
	// Чтобы корректно отослался лог с последнего краша, нужно инциализировать HockeyApp здесь,
	// после того как определен путь, где лежит лог игры, но до того, как этот лог перезапишется новым
	[[BITHockeyManager sharedHockeyManager] configureWithIdentifier:[bundle objectForInfoDictionaryKey:@"HockeyAppID"] delegate:self];
	[[BITHockeyManager sharedHockeyManager] crashManager].crashManagerStatus = BITCrashManagerStatusAutoSend;
	[[BITHockeyManager sharedHockeyManager] startManager];
	[[BITHockeyManager sharedHockeyManager].authenticator authenticateInstallation];
#endif
	
	glView = (EAGLView*)(viewController.view);
	[glView setFrame:[[UIScreen mainScreen] applicationFrame]];
	if ( [window respondsToSelector:@selector(setRootViewController:)] ) {
		[window setRootViewController:viewController];
	} else {
		[window addSubview:[viewController view]];
	}
	[glView startAnimation];
	[window makeKeyAndVisible];

	OnAppLaunched();

	return YES;
}
  
- (void)applicationWillTerminate:(UIApplication *)application
{
	SwrveManager::OnAppTerminate();
	[queue cancelAllOperations];
	[glView stopAnimation];

	OnAppTerminated();
}

- (void) applicationWillResignActive:(UIApplication *)application
{
	[glView stopAnimation];

	OnAppDeactivated();
}

- (void) applicationDidBecomeActive:(UIApplication *)application
{
	if (!queue) {
		dispatch_async(dispatch_get_main_queue(), ^{
			::application->RealLoadResources();
			
			queue = [[NSOperationQueue alloc] init];
			// Загрузка уровней/констант/прочего барахла с playrix.com
			[queue addOperation:[[[NSInvocationOperation alloc] initWithTarget:packageDownloader selector:@selector(downloadThreadOnce) object:nil] autorelease]];
		});
	}
		
	[glView startAnimation];
	SwrveManager::OnAppBecomeActive();
	
	FB::HandleDidBecomeActive();

	if (MM::AudioSession::IsOtherAudioPlaying()) {
		MM::manager.SetMusicVolume(0);
		Core::LuaCallVoidFunction("onWakeUpGame");
	}
	else {
		MM::manager.SetMusicVolume(gameInfo.getMusicVolume()*100.f);
	}
	
	OnAppActivated();
}

- (void)applicationDidEnterBackground:(UIApplication *) application
{
	SwrveManager::OnAppEnterBackground();

	OnAppCollapsed();
}

- (void) applicationWillEnterForeground:(UIApplication *)application
{
	OnAppShown();
}

- (NSString *)applicationLogForCrashManager:(BITCrashManager *)crashManager
{
	std::string logPath = File::GetSpecialFolderPath(SpecialFolder::LocalData) + "/log.txt";
	NSString *logString = [NSString stringWithContentsOfFile:[NSString stringWithUTF8String:logPath.c_str()] encoding:NSUTF8StringEncoding error:nil];
	return logString;
}

//- (void) crashManagerWillSendCrashReport:(BITCrashManager *)crashManager
//{
//	NSLog(@"[HockeySDK] Will send crash report");
//}
//
//- (void) crashManagerDidFinishSendingCrashReport:(BITCrashManager *)crashManager
//{
//	NSLog(@"[HockeySDK] Finished sending crash report");
//}
//
//- (void) crashManager:(BITCrashManager *)crashManager didFailWithError:(NSError *)error
//{
//	NSLog(@"[HockeySDK] Failed to send crash (%@)", error);
//}

- (void) showHelp
{
	@try {
		[helpViewController createView];
		[viewController presentModalViewController:helpViewController animated:YES];
		[helpViewController open];
	}
	@catch(NSException * e) { Assert(false); }
	@catch(...) { Assert(false); }
}

- (void) hideHelp
{
	@try {
		MM::manager.PlaySample("ButtonClick");
		[helpViewController dismissModalViewControllerAnimated:YES];
	}
	@catch(NSException * e) { Assert(false); }
	@catch(...) { Assert(false); }
}

- (void) dealloc
{
	[packageDownloader release];
	[window release];
	[glView release];
	[queue release];
	[helpViewController release];
	[super dealloc];
}

- (void) tryDisableDisplayLinks {
	[glView tryDisableDisplayLinks];
}

- (void) tryEnableDisplayLinks {
	[glView tryEnableDisplayLinks];
}

- (void) update {
	if (application) {
		application->MainLoop();
		if (application->isPaused) {
			application->Draw(); // Всё равно рисуем во время паузы - показывается экран загрузки.
		}
	}
}
// UIApplicationDelegate protocol.
// Asks the delegate to open a resource identified by URL.
// Facebook Single Sign On: For 4.2+ support
- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url
  sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
	if (FB::HandleOpenUrl(url)) { return true; }
	return false;
}

@end

//----------------------------------------------------------------------

void ShowHelp() {
	[instance showHelp];
}

void TryDisableDisplayLinks() {
	[instance tryDisableDisplayLinks];
}

void TryEnabledDisplayLinks() {
	[instance tryEnableDisplayLinks];
}
