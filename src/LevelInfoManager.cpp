#include "stdafx.h"
#include "LevelInfoManager.h"
#include "Match3Gadgets.h"
#include "GameInfo.h"
#include "Utils/Str.h"

#ifndef ENGINE_TARGET_WIN32
#include <json/json.h>
#endif

LevelInfoManager::LevelInfoManager()
	:levelStarted(false)
{}

void LevelInfoManager::LevelInfo::Load(rapidxml::xml_node<> *xml_level_info, int level_num) {
	number = level_num;
	name = Xml::GetStringAttributeOrDef(xml_level_info, "levelName", "test_sound");
	data.Load(xml_level_info);
}

void LevelInfoManager::LoadLevelMap() {
	_levelMap.clear();
	_levelMap.reserve(256);
	Xml::RapidXmlDocument doc("LevelMap.xml");

	rapidxml::xml_node<> *elem = doc.first_node()->first_node("Level");
	while (elem) {
		_levelMap.push_back(LevelInfo());
		LevelInfo &info = _levelMap.back();
		_levelMap.back().Load(elem, _levelMap.size()-1);
		elem = elem->next_sibling("Level");
	}
	
	Layer* layer = Core::guiManager.getLayer("GameLayer");
	if (layer != NULL) {
		layer->getWidget("SelectLevel")->AcceptMessage(Message("ReloadNames"));
	}
}

int LevelInfoManager::GetLevelIndex(const std::string& name) {
	int index = -1;
	size_t count = _levelMap.size();
	for (size_t i = 0; i < count; i++) {
		if (_levelMap[i].name == name) {
			index = (int) i;
			break;
		}
	}
	return (index);
}

std::string LevelInfoManager::GetLevelName(int level) {
	std::string level_name = "";
	if(!gameInfo.getLocalString("DEBUG:EditorUtils::lastLoadedLevel", "").empty()) {
		// пытаемся прочитать имя уровня, заданного в редакторе (это действие в приоритете. для редакторов-дизайнеров)
		level_name = gameInfo.getLocalString("DEBUG:EditorUtils::lastLoadedLevel", "");
		if(!File::Exists("Levels/" + level_name + ".xml")) {
			// пытаемся прочитать имя уровня, установленного в DebugSelectLevel
			GUI::Widget *selectLevel = Core::guiManager.getLayer("GameLayer")->getWidget("SelectLevel");
			selectLevel->AcceptMessage(Message("ReloadNames"));
			selectLevel->AcceptMessage(Message("SetLevel", 0));
			level_name = selectLevel->QueryState(Message("CurrentLevel")).getPublisher();
		}
	}
	if (level == -1){
		level = gameInfo.getLocalInt("current_level", 0);
	}
	if (level < 0) {
		level = 0;
		Log::Warn("Level index is less than zero");
	} else if (level >= (int)_levelMap.size()) {
		level = (int)_levelMap.size() - 1;
		Log::Warn("Level index is greater than the maximum");
	}
	if(level_name.empty()) {
		level_name = _levelMap[level].name;
		if(!File::Exists("Levels/" + level_name + ".xml")) {
			GUI::Widget *selectLevel = Core::guiManager.getLayer("GameLayer")->getWidget("SelectLevel");
			selectLevel->AcceptMessage(Message("ReloadNames"));
			selectLevel->AcceptMessage(Message("SetLevel", 0));
			level_name = selectLevel->QueryState(Message("CurrentLevel")).getPublisher();
			if (level_name.empty()) {
				Assert2(false, "Looks like levels failed to download");
			}
		}
	}
	return level_name;
}

LevelInfoManager::LevelInfo& LevelInfoManager::GetLevelInfo(int level) {
	if (level == -1){
		level = gameInfo.getLocalInt("current_level", 0);
	}
	Assert(0 <= level && level < (int)_levelMap.size());
	return _levelMap[level];
}

int LevelInfoManager::getLevelScore(int levelIndex) {
	return gameInfo.getLocalArrInt("level_score", levelIndex);
}

int LevelInfoManager::getLevelStars(int levelIndex) {
	return gameInfo.getLocalArrInt("level_stars", levelIndex);
}

void LevelInfoManager::setLevelStars(int levelIndex, int count_stars) {
	gameInfo.setLocalArrInt("level_stars", levelIndex, count_stars);
}

void LevelInfoManager::setLevelScore(int score) {
	int level = gameInfo.getLocalInt("current_level", 0);
	if(level < 0) {
		return; //Уровень вне списка, ничего инкрементировать не надо
	}
	int old_score = gameInfo.getLocalArrInt("level_score", level);
	int old_stars = gameInfo.getLocalArrInt("level_stars", level);
	int stars = getStarsForScore(score);

	if (((old_score < score) || (old_stars < stars)) && level >= 0) {
		gameInfo.setLocalArrInt("level_score", level, std::max(score, old_score));
		setLevelStars(level, std::max(stars, old_stars));
		if (level == gameInfo.getLocalInt("extreme_level") && Core::guiManager.isLayerLoaded("CardLayer")) {
			//gameInfo.Save(false); происходит внутри incrementHistory (вместе с парсом)
			Core::guiManager.getLayer("CardLayer")->getWidget("CardWidget")->AcceptMessage(Message("incrementHistory"));
		} else {
			gameInfo.Save(false);
		}
	}
}

void LevelInfoManager::setCurrentLevel(int levelIndex) {
	gameInfo.setLocalString("DEBUG:EditorUtils::lastLoadedLevel", "");
	gameInfo.setLocalInt("current_level", levelIndex);
	Core::guiManager.getLayer("GameLayer")->getWidget("GameField")->AcceptMessage(Message("LoadLevelSettings"));
	if(Core::guiManager.getLayer("GameLayer")->getWidget("SelectLevel"))
	{
		Core::guiManager.getLayer("GameLayer")->getWidget("SelectLevel")->AcceptMessage(Message("OnLoadLevel", "Find"));
	}
}

int LevelInfoManager::getStarsForScore(int score) {
	if( score >= Gadgets::levelSettings.getInt("Star3")) {
		return 3;
	} else if( score >= Gadgets::levelSettings.getInt("Star2")) {
		return 2;
	} else if (score > 0) {
		return 1;
	}
	return 0;
}

int LevelInfoManager::getScoreForStar(int star) {
	Assert(star >=1 && star <= 3);
	return Gadgets::levelSettings.getInt("Star" + utils::lexical_cast(star));
}

bool LevelInfoManager::isTutorialLevel() {

	int level = gameInfo.getLocalInt("current_level", 0);
	if(gameInfo.getLocalInt("extreme_level", 0) != level) {
		return false;
	}
	//Срабатывает только если мы играем в крайний(максимальный) extreme_level уровень.
	LevelInfo &info = GetLevelInfo(level);
	return info.data.getBool("is_tutorial", false);
}

bool LevelInfoManager::isLevelStarted() {
	return levelStarted;
}

DataStoreWithRapid& LevelInfoManager::getCurrentLevelData() {
	LevelInfo &info = GetLevelInfo(gameInfo.getLocalInt("current_level"));
	return info.data;
}

std::string LevelInfoManager::getCurrentLevelGoal()
{
	const std::string levelObj = Gadgets::levelSettings.getString("LevelObjective", "receivers");

	if( levelObj == "receivers" )
	{
		int amount = Gadgets::totalReceivers;
		Assert(amount >= 1);
		std::string goal;
		if (amount == 1) {
			goal = Core::resourceManager.Get<Render::Text>("ActivateOneReceiver")->ToString();
		} else if (amount < 5) {
			goal = Core::resourceManager.Get<Render::Text>("Activate2Receivers")->ToString();
		} else {
			goal = Core::resourceManager.Get<Render::Text>("ActivateManyReceivers")->ToString();
		}
		std::string r = "<amount>";
		size_t index = goal.find(r);
		if (index != std::string::npos) {
			goal.replace(index, r.length(), utils::lexical_cast(amount));
		}
		return goal;
	}
	else if( levelObj == "score" )
	{
		int points = Gadgets::levelSettings.getInt("LevelObjectiveAmount");
		std::string goal = Core::resourceManager.Get<Render::Text>("EarnPoints")->ToString();
		std::string r = "<points>";
		size_t index = goal.find(r);
		if (index != std::string::npos) {
			goal.replace(index, r.length(), utils::lexical_cast(points));
		}
		return goal;
	}
	else if( levelObj == "energy" )
	{
		return Core::resourceManager.Get<Render::Text>("FillEnergy")->ToString();
	}
	else if( levelObj == "diamonds" )
	{
		return Core::resourceManager.Get<Render::Text>("CollectDiamonds")->ToString();
	}
	else if( levelObj == "bears" )
	{
		return Core::resourceManager.Get<Render::Text>("FindBears")->ToString();
	}
	return "Unknown level type: " + levelObj;
}

void LevelInfoManager::OnABInfo(const char *data)
{
#ifndef ENGINE_TARGET_WIN32
	Json::Reader reader;
	Json::Value ab_json;
	reader.parse(data, ab_json, false);

	if (!ab_json.isArray()) { Assert(false); return; }

	for(Json::Value::iterator jit = ab_json.begin(); jit != ab_json.end(); ++jit) {
		const Json::Value& resource = *jit;

		// Получаем id ресурса
		const Json::Value& id_val = resource["uid"];
		if (id_val.isNull()) { Assert(false); continue; }
		std::string id = id_val.asString();
		if (id.empty()) { Assert(false); continue; }
		
		// Формируем словарь значений, содержащихся в ресурсе
		Dict dict;
		for (Json::Value::const_iterator it = resource.begin(); it != resource.end(); ++it) {
			// Пропускаем служебные атрибуты
			if (utils::equals(it.memberName(), "name")) { continue; }
			if (utils::equals(it.memberName(), "thumbnail")) { continue; }
			if (utils::equals(it.memberName(), "description")) { continue; }
			if (utils::equals(it.memberName(), "item_class")) { continue; }
			if (utils::equals(it.memberName(), "uid")) { continue; }
			
			const Json::Value& v = resource[it.memberName()];
			dict[it.memberName()] = v.asString();
		}

		// Применяем полученнный ресурс где ему положену по его типу (в данный момент это только данные баланса уровней)
		if( utils::String::IsHeadCut(id, "settings.m3balance.level.")) {
			_levelSettingsAB[id] = dict;
		}
	}
#endif
}

LevelInfoManager::Dict LevelInfoManager::GetLevelSettingsAB(const std::string& levelName)
{
	return _levelSettingsAB[levelName];
}

size_t LevelInfoManager::GetSize()
{
	return _levelMap.size();
}

LevelInfoManager levelsInfo;