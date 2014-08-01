#include "stdafx.h"
#include "LiveCounter.h"
#include "GameInfo.h"
#include "Marketing/LocalNotifications.h"
#include "SwrveManager.h"
#include "MyApplication.h"

LiveCounter::LiveCounter()
	:liveAmount(0)
	,timeToNextLive(0)
{
	GENERATE_TIME = gameInfo.getConstInt("LiveGenerateTime");
	MAX_GENERATE_LIVES = gameInfo.getConstInt("MaxLives");
}

void LiveCounter::load() {
	liveAmount = gameInfo.getLocalInt("liveAmount");
	nextGenerateTime = gameInfo.getLocalDouble("nextLive", gameInfo.getGlobalTime());
	update(0.f);
}

void LiveCounter::save() {
	gameInfo.setLocalInt("liveAmount", liveAmount);
	gameInfo.setLocalDouble("nextLive", nextGenerateTime);
	gameInfo.Save(false);
}

void LiveCounter::update(float dt) {
	if (liveAmount < MAX_GENERATE_LIVES) {
		int liveChange = 0;
		while (gameInfo.getGlobalTime() > nextGenerateTime && liveAmount < MAX_GENERATE_LIVES) {
			// генерация жизни
			++liveAmount;
			++liveChange;
			nextGenerateTime += GENERATE_TIME;
		}
		if (liveChange > 0) {
			Core::messageManager.putMessage(Message("lua:LiveCounterChange", liveAmount));
			SwrveManager::TrackEvent("Live.Get.Timer", 
				"{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("extreme_level")) +
				",\"LiveCount\":" + utils::lexical_cast(liveChange) + "}"
			);
		}
		timeToNextLive = static_cast<int>(nextGenerateTime - gameInfo.getGlobalTime());
	} else {
		timeToNextLive = 0;
	}
}

bool LiveCounter::spendLive() {
	if (liveAmount > 0) {
		if (liveAmount == MAX_GENERATE_LIVES) {
			timeToNextLive = GENERATE_TIME;
			nextGenerateTime = gameInfo.getGlobalTime() + GENERATE_TIME;
		}
		--liveAmount;
		gameInfo.updateLocalNotifications();
		save();
		Core::messageManager.putMessage(Message("lua:LiveCounterChange", liveAmount));
		SwrveManager::TrackPurchase("live.lost", 1, SwrveManager::LIVE_CURRENCY);
		return true;
	}
	return false;
}

void LiveCounter::addLive() {
	if (liveAmount < MAX_GENERATE_LIVES) {
		++liveAmount;
		if (liveAmount >= MAX_GENERATE_LIVES) {
			timeToNextLive = 0;
		}
		gameInfo.updateLocalNotifications();
		save();
		Core::messageManager.putMessage(Message("lua:LiveCounterChange", liveAmount));
		SwrveManager::TrackPurchase("live.from_friends", 1, SwrveManager::LIVE_CURRENCY);
		SwrveManager::TrackEvent("Live.Get.FromFirends", 
			"{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("extreme_level")) +
			",\"EmptyLive\":" + utils::lexical_cast(MAX_GENERATE_LIVES - liveAmount + 1) + "}"
		);
	}
}


void LiveCounter::fillLives() {
	int buy_lives = MAX_GENERATE_LIVES - liveAmount;
	liveAmount = MAX_GENERATE_LIVES;
	gameInfo.updateLocalNotifications();
	Core::messageManager.putMessage(Message("lua:LiveCounterChange", liveAmount));
	save();

	SwrveManager::TrackPurchase("live.buy", buy_lives , SwrveManager::LIVE_CURRENCY);
	SwrveManager::TrackEvent("Purchase.Lives",
		"{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("extreme_level")) + 
		",\"EmptyLive\":" + utils::lexical_cast(buy_lives) + "}");
}

bool LiveCounter::notifications() const {
	return gameInfo.getLocalBool("fillLiveNotification", true);
}

void LiveCounter::notificationsEnable(bool enable) {
	if (enable != gameInfo.getLocalBool("fillLiveNotification", true)) {
		gameInfo.setLocalBool("fillLiveNotification", enable);
		gameInfo.updateLocalNotifications();
		SwrveManager::TrackEvent("UserInfo.Notifications", std::string("{\"Choise\":\"") + (enable ? "Yes" : "No") + "\"}");
	}
}

void LiveCounter::addNotification() {
	if (gameInfo.getLocalBool("fillLiveNotification", true) && liveAmount < MAX_GENERATE_LIVES) {
		Marketing::LocalNotifications::Add(
			static_cast<int>(nextGenerateTime - gameInfo.getGlobalTime()) // кол-во времени до следующей жизни
			+ (MAX_GENERATE_LIVES - liveAmount - 1) * GENERATE_TIME,         // кол-во недостоющих жизней * время генерации одной жизни
			"FullLiveNotificationText"
		);
	}
}