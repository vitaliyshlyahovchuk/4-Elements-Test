#include "stdafx.h"
#include "GameInfo.h"
#include "Map.h"
#include "MapItem.h"
#include "LevelMarker.h"
#include "Gateway.h"
#include "CardFactory.h"
#include "AvatarContainer.h"
#include "Shine.h"
#include "SwrveManager.h"
#include "MyApplication.h"
#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
#include "ParseManager.h"
#endif

using namespace Card;

std::string NumberToStr(int num, int digits = 3)
{
	std::string str = utils::lexical_cast(num);
	Assert(static_cast<int>(str.length()) <= digits);
	std::string zeros(digits - str.length(), '0');
	return zeros + str;
}

Map::Map()
	:Container()
	,userAvatar(NULL)
	,shine(new Shine())
{
}

Map::~Map()
{
	delete shine;
}

bool markerSortPredicate(UpdateLevelItem* a, UpdateLevelItem* b) {
	return a->getMarker() < b->getMarker();
}

void Map::removeChildren() {
	Container::removeChildren();
	markers.clear();
	avatar_friends.clear();
	level_markers.clear();
	gate_markers.clear();
	userAvatar = NULL;
}

void Map::loadDescription(rapidxml::xml_node<>* objectXml, rapidxml::xml_node<>* shineXml, bool debugLoad) {
	removeChildren();

	for (rapidxml::xml_node<>* e = objectXml->first_node("item"); e; e = e->next_sibling("item")) {
		MapItem * item = Factory::create(this, e);
		children.push_back(item);

		if (item->GetType() == "LevelMarker") {
			markers.push_back(dynamic_cast<UpdateLevelItem*>(item));
			avatar_friends.push_back(NULL);
			level_markers.push_back(dynamic_cast<LevelMarker*>(item));
		} else
		if (item->GetType() == "Gateway") {
			markers.push_back(dynamic_cast<UpdateLevelItem*>(item));
			avatar_friends.push_back(NULL);
			gate_markers.push_back(dynamic_cast<Gateway*>(item));
		}
	}

	std::sort(markers.begin(), markers.end(), &markerSortPredicate);
	std::sort(gate_markers.begin(), gate_markers.end(), &markerSortPredicate);

	shine->init(shineXml);
}

void Map::initEpisodShine() {
	int episod_index = 0;
	int current_marker = gameInfo.getLocalInt("current_marker");
	for (int i = 0, len = static_cast<int>(gate_markers.size()); i < len; ++i) {
		if (current_marker > gate_markers[i]->getMarker()) {
			episod_index = i + 1;
		} else {
			break;
		}
	}
	shine->InitEpisods(episod_index);
}

bool Map::isLoaded() const {
	return children.size() > 0;
}

UpdateLevelItem* Map::getUpdateLevelItem(int level_marker) {
	Assert(0 <= level_marker && level_marker < static_cast<int>(markers.size()));
	return markers[level_marker];
}

CardAvatarFriends* Map::getAvatarFriends(int level_marker) {
	Assert(0 <= level_marker && level_marker < static_cast<int>(avatar_friends.size()));
	CardAvatarFriends* item = avatar_friends[level_marker];
	if (!item) {
		item = new CardAvatarFriends();
		avatar_friends[level_marker] = item;
		AddChild(item);
		item->attachToMarker(markers[level_marker]);
	}
	return item;
}


void Map::updateUserAvatar() {
	if (userAvatar == NULL) {
		userAvatar = new CardUserAvatar(gameInfo.getAvatar());
		AddChild(userAvatar);
	}
	userAvatar->attachToMarker(getUpdateLevelItem(gameInfo.getLocalInt("current_marker")));
}

void Map::runUserAvatar() {
	Assert(userAvatar);
	userAvatar->goToMarker(getUpdateLevelItem(gameInfo.getLocalInt("current_marker")));
}

void Map::clearAvatarFriends() {
	for (size_t i = 0, len = avatar_friends.size(); i < len; ++i) {
		if (avatar_friends[i]) {
			avatar_friends[i]->ClearFriends();
		}
	}
}

void Map::correct(int &index) {
	if (index < 0) {
		index = 0;
		return;
	}
	int size = static_cast<int>(markers.size());
	if (index >= size) {
		index = size - 1;
	}
}

void Map::openEpisod() {
	shine->OpenEpisod();
}

void Map::debugTryOpenEpisod() {
	if (inGateway()) {
		shine->OpenEpisod();
	}
}

void Map::incrementHistory() {
	int marker = gameInfo.getLocalInt("current_marker");
	marker += 1;
	if (marker < static_cast<int>(markers.size())) {
		gameInfo.setLocalInt("current_marker", marker);
		UpdateLevelItem *item = getUpdateLevelItem(marker);
		if (item->GetType() == "LevelMarker") {
			gameInfo.setLocalInt("extreme_level", gameInfo.getLocalInt("extreme_level") + 1);
			// проверка на расхождение истории
			Assert(dynamic_cast<LevelMarker*>(item)->GetLevel() == gameInfo.getLocalInt("extreme_level"));
			//статистика
			SwrveManager::TrackEvent("Level.NewLevel." + NumberToStr(gameInfo.getLocalInt("extreme_level")), 
				"{\"AmountCrystals\":" + utils::lexical_cast(gameInfo.getCash()) +
				",\"payers\":" + (gameInfo.getLocalBool("payer") ? "true" : "false") +
				"}");
		} else {
			SwrveManager::TrackEvent(
				"Level.StartTimer.Gate_" + NumberToStr((gameInfo.getLocalInt("extreme_level") + 1) / 15, 2),
				std::string("{\"FBConnected\":") + (GameLua::FBIsLoggedIn() ? "true" : "false") + "}"
			);
		}
	} else {
		gameInfo.setLocalBool("isEndHistory", true); // переменная, которая будет показывать то, что игрок дошел до конца всех уровней
	}
#ifndef ENGINE_TARGET_WIN32
	gameInfo.Save(false);
	ParseManager::Save();
#endif
}

void Map::debugDecrementHistory() {
	int marker = gameInfo.getLocalInt("current_marker");
	marker -= 1;
	if (marker >= 0) {
		gameInfo.setLocalInt("current_marker", marker);
		UpdateLevelItem *item = getUpdateLevelItem(marker + 1);
		if (item->GetType() == "LevelMarker") {
			gameInfo.setLocalInt("extreme_level", gameInfo.getLocalInt("extreme_level") - 1);
		}
		// очистка звезд и очков выше current_marker-а
		item = markers[markers.size() - 1];
		if (item->GetType() == "Gateway") {
			item = markers[markers.size() - 2];
		}
		int max_level = dynamic_cast<LevelMarker*>(item)->GetLevel();
		for (int i = gameInfo.getLocalInt("extreme_level"); i <= max_level; ++i) {
			gameInfo.setLocalArrInt("level_score", i, 0);
			gameInfo.setLocalArrInt("level_stars", i, 0);
		}
		gameInfo.setLocalBool("isEndHistory", false);
		
#ifndef ENGINE_TARGET_WIN32
		gameInfo.Save(false);
		ParseManager::Save();
#endif
	}
}

void Map::initMarkers() {
	for (size_t i = 0; i < markers.size(); ++i) {
		markers[i]->InitMarker();
	}
	initEpisodShine();
}
bool Map::inGateway() {
	return markers[gameInfo.getLocalInt("current_marker")]->GetType() == "Gateway";
}

void Map::runStarEffects() {
	for (std::list<LevelMarker*>::iterator i = level_markers.begin(), end = level_markers.end(); i != end; ++i) {
		(*i)->runStarEffects();
	}
}

void Map::removeFrozenAnimations() {
	for (std::vector<Gateway*>::iterator i = gate_markers.begin(), end = gate_markers.end(); i != end; ++i) {
		(*i)->removeFrozenAnimation();
	}
}

void Map::UpdateShine(float dt) {
	shine->Update(dt);
}
void Map::DrawShine(RenderTargetHolder* target, float shift) {
	shine->Draw(target, shift);
}
void Map::DrawShineEffect(float shift) {
	shine->DrawEffect(shift);
}