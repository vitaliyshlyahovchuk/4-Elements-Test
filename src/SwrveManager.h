#ifndef __SWRVEMANAGER_H__
#define __SWRVEMANAGER_H__

#pragma once


class SwrveManager
{
public:
	static const std::string ALL_CURRENCY;
	static const std::string REAL_CURRENCY;
	static const std::string LIVE_CURRENCY;
	
	static void TrackEvent(const std::string& event_name, const std::string& payload);
	static void TrackPurchase(const std::string& item, int cost, const std::string& currency, int quantity = 1);
	static void Start();
	static void TrackBuyCurrency(const std::string &currencyPackId);
	static void TrackCurrencyGiven(const std::string &currency, int amount);
	static void OnAppEnterBackground();
	static void OnAppBecomeActive();
	static void OnAppTerminate();
private:
	static bool enableLogging();
};

#endif // __SWRVEMANAGER_H__
