#ifndef _COMMON_APP_COMMON_
#define _COMMON_APP_COMMON_

void OpenURL(const std::string& url);
void OpenFacebookPage();
void CheckNetworkStatus();
void FacebookSilentLogout();

std::string GetAppVersion();
std::string GetBuildNumberStr();
int GetBuildNumber();

void OnAppLaunched(); // onCreate, didFinishLaunchingWithOptions
void OnAppTerminated(); // onDestroy, applicationWillTerminate

void OnAppActivated(); // onResume, applicationDidBecomeActive
void OnAppDeactivated(); // onPause, applicationWillResignActive

void OnAppShown(); // onStart, applicationWillEnterForeground
void OnAppCollapsed(); // onStop, applicationDidEnterBackground

#endif //_COMMON_APP_COMMON_
