#include "stdafx.h"

#include "GameInfo.h"
#include "MyApplication.h"
#include "LiveCounter.h"
#include "FriendInfo.h"
#include "Marketing/LocalNotifications.h"
#include "LevelInfoManager.h"
#include "SwrveManager.h"

#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
	#include "ParseManager.h"
	#include "MicroTransactions.h"
#endif

void SimpleCypher(char * data, size_t size); // функция описана в GameScriptMap.cpp
void UpdateGameInfoHash(std::string data); // функция описана в AppDelegate.mm или WinMain.cpp
bool hasModificationGameInfo(std::string data); // функция описана в AppDelegate.mm или WinMain.cpp
std::string GetUniqueDeviceIdentifier(); // функция описана в MyEngineWrapper

GameInfo::GameInfo()
	: _liveCounter(NULL)
	, avatar(NULL)
	, timeSaveToParse(0.0)
	, lastChangeCashValue(0)
{
	Update(0.f);
}

GameInfo::~GameInfo()
{
	delete avatar;
}

bool GameInfo::IsDevMode()
{
	return getLocalInt("user_type") == 1;
}

void GameInfo::ReloadConstants() {
	Xml::RapidXmlDocument doc("GameSettings.xml");
	rapidxml::xml_node<>* xe = doc.first_node();
	LoadConstData(xe->first_node("constants"));
}

void GameInfo::Init() {
	Xml::RapidXmlDocument doc("GameSettings.xml");
	rapidxml::xml_node<>* xe = doc.first_node();
	LoadConstData(xe->first_node("constants"));
	LoadGlobalData(xe->first_node("variables"));

	// загрузка цен в магазин

	prices.clear();
	std::string info_prefix = "purchase_";
	for (int i = 1; i < 6; ++i) {
		std::string id = std::string("p") + utils::lexical_cast(i);
		if (_consts.findName(info_prefix + id)) {
			prices.insert(std::make_pair(id, Price(id, getConstInt(info_prefix + id))));
		}
	}
#ifndef ENGINE_TARGET_WIN32
	std::vector<std::string> store_purchases;
	std::string purchase_prefix = Core::GlobalConstants::GetString("purchase_prefix");
	for (std::map<std::string, Price>::iterator i = prices.begin(), end = prices.end(); i != end; ++i) {
		store_purchases.push_back(purchase_prefix + i->first);
	}
	InApp::InitStore(store_purchases, false);
#endif
}

void GameInfo::LoadFromFile(bool newSave)
{
	const std::string filename = "GameInfo.xml";
	std::vector<uint8_t> xmlData;
	if (!File::Exists(filename) || newSave) {
		// первый запуск, приложение удаляли с устройства, либо новая сохранка
		File::LoadFile("GameInfoDefault.xml", xmlData);
		std::vector<char> encrypted_data(xmlData.begin(), xmlData.end());
		SimpleCypher(&encrypted_data[0], encrypted_data.size());
		Core::fileSystem.OpenWrite(filename)->Write(&encrypted_data[0], encrypted_data.size());
	} else {
		File::LoadFile(filename, xmlData);
		if (getGlobalBool("SoftCypher")) {
			// в режиме "мягкого" шифрования, проверяем, заршифрован ли файл (на винде можно расшифровать файл, и обратно не шифровать)
			if ((char)xmlData[0] != '<') {
				SimpleCypher((char*)&xmlData[0], xmlData.size());
			}
		} else {
			// шифрование включено всегда
			SimpleCypher((char*)&xmlData[0], xmlData.size());
		}
	}
	xmlData.push_back(0);
	bool hasModification = !newSave && hasModificationGameInfo(std::string((char*)&xmlData[0]));
	rapidxml::xml_document<char> doc;
	doc.parse<rapidxml::parse_default>((char*)&xmlData[0]);
	LoadFromXml(doc.first_node());
	levelsInfo.LoadLevelMap();
	if (hasModification) {
		setPerhapsCheater();
	}
	LoadBoostTutorials();
}

void GameInfo::LoadFromXml(rapidxml::xml_node<>* xml_root) {
	
	playerInfo.Load(xml_root);

	LoadShownBoostTutorials(xml_root);

	// установим громкость
	setSoundVolume(getSoundVolume());
	setMusicVolume(getMusicVolume());
	// счетчик жизней
	if (_liveCounter == NULL) {
		_liveCounter = new LiveCounter();
	}
	_liveCounter->load();
	// время последнего сохранения
	timeSaveToParse = getLocalDouble("timeSaveToParse", 0.0);
	// в момент загрузки, если нужно сохранить прошлые изменения, откладываем сохранение на одну минуту
	if (timeSaveToParse > 0.0) {
		timeSaveToParse = std::max(timeSaveToParse, getGlobalTime() + 60.0);
	}
//~~#if defined(ENGINE_TARGET_WIN32) || !(defined(ENTERPRISE) || defined(APPSTORE))
	// в винде, не в ENTERPISE и не в APPSTORE билде сделаем всех игроков
	// разработчиками, но только при первом запуске, чтобы можно было изменить и протестировать
	// без DevMode
	if (!playerInfo.findName("user_type")) {
		setLocalInt("user_type", 1); // 1 - разработчик
	}
//~~#endif
}

void GameInfo::LoadDefaultVariables() {
	setLocalInt("liveAmount", getConstInt("MaxLives"));
	setLocalDouble("nextLive", 0.0);
}

void GameInfo::Save(bool parseSave)
{
	if (parseSave && timeSaveToParse == 0.0) {
		timeSaveToParse = getGlobalTime() + getConstDouble("SaveParseTime");
		setLocalDouble("timeSaveToParse", timeSaveToParse);
	}
	
#ifndef ENGINE_TARGET_WIN32
	// нужно всегда обноавлять, т.к. при обновлении системы ios c 6 на 7 возможно изменение device id
	setLocalString("lastDeviceId", GetUniqueDeviceIdentifier());
#endif

	rapidxml::xml_document<char> doc;
	rapidxml::xml_node<char>* root = Xml::NewNode(&doc, "root");

	playerInfo.Save(root);

	SaveShownBoostTutorials(root);

	// Записываем в строку.
	std::string data;
	rapidxml::print(std::back_inserter(data), doc, 0);
	// обновляем хеш, для поиска читеров
	UpdateGameInfoHash(data);
	// Шифруем.
	SimpleCypher(&data[0], data.size());
	// Записываем в файл
	Core::fileSystem.OpenWrite("GameInfo.xml")->Write(&data[0], data.size());
	Log::Info("gameInfo saved");
}

void GameInfo::parseSaved() {
	if (timeSaveToParse != 0.0) {
		timeSaveToParse = 0.0;
		setLocalDouble("timeSaveToParse", timeSaveToParse);
	}
}

bool GameInfo::localNameDefined(const std::string& name) {
	return playerInfo.findName( name );
}

std::string  GameInfo::getLocalProperty(const std::string  &name, const std::string  &def) {
	return playerInfo.getString(name, def);
}

void GameInfo::setLocalProperty(const std::string  &name, const std::string  &value) {
	playerInfo.setString(name, value);
}

int GameInfo::getLocalInt(const std::string& name, int defaultValue) {
	return playerInfo.getInt(name, defaultValue);
}

bool GameInfo::getLocalBool(const std::string& name, bool defaultValue) {
	return playerInfo.getBool(name, defaultValue);
}

float GameInfo::getLocalFloat(const std::string& name, float defaultValue) {
	return playerInfo.getFloat(name, defaultValue);
}

double GameInfo::getLocalDouble(const std::string& name, double defaultValue) {
	return playerInfo.getDouble(name, defaultValue);
}

std::string  GameInfo::getLocalString(const std::string& name, std::string defaultValue) {
	return playerInfo.getString(name, defaultValue);
}

void GameInfo::setLocalInt(const std::string& name, int value) {
	playerInfo.setInt(name, value);
}

void GameInfo::setLocalBool(const std::string& name, bool value) {
	playerInfo.setBool(name, value);
}

void GameInfo::setLocalFloat(const std::string& name, float value) {
	playerInfo.setFloat(name, value);
}

void GameInfo::setLocalDouble(const std::string& name, double value) {
	playerInfo.setDouble(name, value);
}

void GameInfo::setLocalString(const std::string& name, const std::string& value) {
	playerInfo.setString(name, value);
}

int GameInfo::getLocalArrInt(const std::string& name, int index) {
	return playerInfo.getArrInt(name, index);
}

void GameInfo::setLocalArrInt(const std::string& name, int index, int value) {
	playerInfo.setArrInt(name, index, value);
}

void GameInfo::eraseLocalVariable(const std::string& name) {
	playerInfo.erase(name);
}

void GameInfo::setMusicVolume(float volume) {
	MM::manager.SetMusicVolume(volume * 100.f);
	setLocalFloat("MusicVolume", volume);
}

void GameInfo::setSoundVolume(float volume) {
	MM::manager.SetSoundVolume(volume * 100.f);
	setLocalFloat("SoundVolume", volume);
}

float GameInfo::getMusicVolume() {
	return getLocalFloat("MusicVolume", 0.0f);
}

float GameInfo::getSoundVolume() {
	return getLocalFloat("SoundVolume", 0.0f);
}

float GameInfo::GetTimeText(Render::Text* text)
{
	float time = 0;
	if (text != NULL)
	{
	int numChars = 0;
		numChars = text->ToString().size();
		//for (size_t i = 0; i < text->GetLines().size(); i++)
		//{
		//    const std::vector<TWord>& line = text->GetLines()[i].GetWords();
		//    for (size_t j = 0; j < line.size(); j++)
		//    {
		//        numChars += static_cast<int> (line[j].ToString().length());
		//    }
		//}

		float kLocal = getGlobalFloat("text_speed_" + Core::locale.GetLanguage(), 1.f);

		if (numChars < 20) 
		{
			time = kLocal*numChars/5.f;
		} else if (numChars < 50) {
			time = kLocal*numChars/10.f;
		} else {
			time = kLocal*numChars*0.09f;
		}
	}
	return time;
}

float GameInfo::GetTimeText(const std::string &text_id)
{
	if(!Core::resourceManager.Exists<Render::Text>(text_id))
	{
		return 0.f;
	}
	return GetTimeText(Core::resourceManager.Get<Render::Text>(text_id));
}

double global_time(0.f);

double GameInfo::getGlobalTime() {
	return global_time;
}

void GameInfo::Update(float dt) {
#if defined ( ENGINE_TARGET_WIN32 )
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	global_time = (static_cast<__int64>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10000000.0;
#elif defined ( ENGINE_TARGET_IPHONE ) || defined(ENGINE_TARGET_LINUX)
	struct timeval time;
	gettimeofday(&time, NULL);
	global_time = time.tv_sec + (double)time.tv_usec / 1000000.0;
#else
	Assert2(false);
#endif
	if (_liveCounter) {
		_liveCounter->update(dt);
	}
	
	if (timeSaveToParse != 0.0 && timeSaveToParse < global_time) {
		timeSaveToParse = 0.0;
		setLocalDouble("timeSaveToParse", timeSaveToParse);
		Save(false);
#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
		ParseManager::Save();
	}
	ParseManager::Update(dt);
#else 
	}
#endif
}

std::string GameInfo::GetUUID(unsigned int amount)
{
	// время
	time_t t = std::time(0);
	char bufferTime[64];
	strftime(bufferTime, sizeof(bufferTime) / sizeof(bufferTime[0]), "%d%m%Y-%H%M%S", localtime(&t));
	
	// 3 случайных символа
	static std::string s = "01234565789qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";
	std::string result("-");
	for (unsigned int i = 0; i < amount; ++i) {
		result += s[math::random(0u, s.size()-1)];
	}
	return std::string(bufferTime) + result;
}

UserAvatar* GameInfo::getAvatar() {
	return avatar ? avatar : (avatar = new UserAvatar());
}

FriendInfo* GameInfo::getFriendByUid(const std::string facebookUid) {
	Assert(facebookUid != "");
	for (size_t i = 0, size = friends.size(); i < size; ++i) {
		if (friends[i]->fb_id == facebookUid) {
			return friends[i];
		}
	}
	return NULL;
}

void GameInfo::updateLocalNotifications() {
	// удалим все
	Marketing::LocalNotifications::CancelAll();
	// добавим те, которые требуются
	_liveCounter->addNotification();
}

void GameInfo::setPerhapsCheater() {
	if (getLocalInt("user_type", 0) == 0) { // переменная имет название user_type, чтобы мошенники не догадались, что тут идентификация мошенников
		setLocalInt("user_type", 2); // тип игрока (0 - простые игроки, 1 - разработчики, 2 - читеры)
	}
}

void GameInfo::buy(std::string productId) {
	Assert(prices.find(productId) != prices.end());
	Price& price = prices[productId];
	addCash(price.cash, true);
	Save(true);
	SwrveManager::TrackPurchase("currency_given.iap", price.cash, SwrveManager::ALL_CURRENCY);
}

int GameInfo::getCash() {
	return getLocalInt("free_cash") + getLocalInt("real_cash");
}

void GameInfo::spendCash(int value, ChangeCashHandler callback, const std::string& purchaseId) {
	Assert(value > 0 && getCash() >= value);
	lastChangeCashValue = value;
	lastChangeCashId = purchaseId;
	lastChangeCashCallback = callback;
	Core::LuaCallVoidFunction("onStartChangeCash");
#ifdef ENGINE_TARGET_WIN32
	_spendCash(true);
#else
	ParseManager::RefreshCash(true); // ParseManager вызовет _spendCash
#endif
}

void GameInfo::_spendCash(bool successfulRefresh) {
	if (successfulRefresh) {
		if (lastChangeCashValue <= getCash()) {
			Core::LuaCallVoidFunction("onEndChangeCash",  "success");
			lastChangeCashCallback();
			purchaseHistory.push_back(PurchaseHistory(lastChangeCashValue, lastChangeCashId, getGlobalTime()));
			int value = lastChangeCashValue;
			int free_cash = getLocalInt("free_cash");
			int spend_real_value = 0;
			if (free_cash > 0) {
				if (free_cash >= value) {
					setLocalInt("free_cash", free_cash - value);
					value = 0;
				} else {
					setLocalInt("free_cash", 0);
					spend_real_value = value - free_cash;
					value -= free_cash;
				}
			}
			if (value > 0) {
				spend_real_value += value;
				setLocalInt("real_cash", getLocalInt("real_cash") - value);
			}
			Core::LuaCallVoidFunction("updateCashView", -lastChangeCashValue);
			// статистика
			SwrveManager::TrackPurchase("purchase." + lastChangeCashId, lastChangeCashValue, SwrveManager::ALL_CURRENCY, 1);
			if (spend_real_value > 0) {
				SwrveManager::TrackPurchase("purchase." + lastChangeCashId, spend_real_value, SwrveManager::REAL_CURRENCY, 1);
			}
		} else {
			Core::LuaCallVoidFunction("onEndChangeCash",  "spend_no_money");
		}
	} else {
		Core::LuaCallVoidFunction("onEndChangeCash", "spend_no_connection");
	}
	lastChangeCashValue = 0;
	lastChangeCashId = "";
	lastChangeCashCallback = NULL;
}

void GameInfo::addCash(int value, bool realCash, ChangeCashHandler callback) {
	Assert(value > 0);
	lastChangeCashValue = value;
	lastChangeCashId = realCash ? "real" : "free";
	lastChangeCashCallback = callback;
#ifdef ENGINE_TARGET_WIN32
	_addCash(true);
#else
	ParseManager::RefreshCash(false); // ParseManager вызовет _addCash
#endif
}

void GameInfo::_addCash(bool successfulRefresh) {
	if (successfulRefresh) {
		Core::LuaCallVoidFunction("onEndChangeCash", "success");
		if (lastChangeCashCallback) {
			lastChangeCashCallback();
		}
		int value = lastChangeCashValue;
		bool realCash = lastChangeCashId == "real";
		purchaseCashHistory.push_back(PurchaseCashHistory(value, realCash, getGlobalTime()));
		if (realCash) {
			setLocalInt("real_cash", getLocalInt("real_cash") + value);
		} else {
			setLocalInt("free_cash", getLocalInt("free_cash") + value);
		}
		Core::LuaCallVoidFunction("updateCashView", value);
	} else {
		Core::LuaCallVoidFunction("onEndChangeCash", "add_no_connection");
	}
	lastChangeCashValue = 0;
	lastChangeCashId = "";
	lastChangeCashCallback = NULL;
}

void GameInfo::LoadBoostTutorials()
{
	boostTutorials.clear();

	Xml::RapidXmlDocument doc("BoostTutorials.xml");

	rapidxml::xml_node<> *tutorial = doc.first_node()->first_node("BoostTutorial");
	while (tutorial) {
		std::string name = Xml::GetStringAttributeOrDef(tutorial, "name", "");
		int level = Xml::GetIntAttributeOrDef(tutorial, "level", 0);
		int type = Xml::GetIntAttributeOrDef(tutorial, "type", 0);
		std::vector<IPoint> points;
		rapidxml::xml_node<> *pointNode = tutorial->first_node("point");
		while (pointNode) {
			int x = Xml::GetIntAttributeOrDef(pointNode, "x", 0);
			int y = Xml::GetIntAttributeOrDef(pointNode, "y", 0);
			points.push_back(IPoint(x, y));
			pointNode = pointNode->next_sibling("point");
		}
		boostTutorials.insert(std::make_pair(level, BoostTutorial(name, level, type, points)));
		tutorial = tutorial->next_sibling("BoostTutorial");
	}
}

bool GameInfo::wasBoostTutorialShown(std::string name)
{
	bool found = false;
	for(int i = 0; i < (int)shownBoostTutorials.size(); i++) {
		if (shownBoostTutorials[i] == name) {
			found = true;
			break;
		}
	}
	return found;
}

bool GameInfo::needBoostTutorial(int level, int type)
{
	bool need = false;
	for (std::map<int, BoostTutorial>::iterator i = boostTutorials.begin(), e = boostTutorials.end(); i != e; ++i) {
		if (i->second.level == level && i->second.type == type) {
			if (!wasBoostTutorialShown(i->second.name)) {
				currentBoostTutorial = i->second;
				need = true;
			}
			break;
		}
	}
	if (!need) currentBoostTutorial = BoostTutorial();
	return need;
}

void GameInfo::setBoostTutorialShown(std::string name)
{
	// TODO: возможно стоит проверять уникальность
	shownBoostTutorials.push_back(name);
}

std::string GameInfo::getBoostTutorialName()
{
	return currentBoostTutorial.name;
}

bool GameInfo::allowBoostTutorial(std::string name, int level)
{
	bool allow = false;

	std::map<int, BoostTutorial>::iterator i = boostTutorials.find(level);
	if (i != boostTutorials.end()) {
		if (i->second.name == name) {
			allow = true; // сейчас туториал об этом бусте
		}
	}

	if (wasBoostTutorialShown(name)) {
		allow = true; // уже был показан
	}

	return allow;
}

void GameInfo::SaveShownBoostTutorials(rapidxml::xml_node<>* elem)
{
	rapidxml::xml_node<> *shownBoosts = Xml::NewNode(elem, "ShownBoostTutorials");
	for(int i = 0; i < (int)shownBoostTutorials.size(); i++) {
		rapidxml::xml_node<> *shownBoost = Xml::NewNode(shownBoosts, "ShownBoost");
		Xml::SetStringAttribute(shownBoost, "name", shownBoostTutorials[i]);
	}
}

void GameInfo::LoadShownBoostTutorials(rapidxml::xml_node<>* elem)
{
	shownBoostTutorials.clear();
	rapidxml::xml_node<>* shownBoosts = elem->first_node("ShownBoostTutorials");
	if (shownBoosts) {
		rapidxml::xml_node<>* shownBoost = shownBoosts->first_node("ShownBoost");
		while (shownBoost) {
			std::string name = Xml::GetStringAttribute(shownBoost, "name");
			shownBoostTutorials.push_back(name);
			shownBoost = shownBoost->next_sibling("ShownBoost");
		}
	}
}

void GameInfo::ResetShownBoostTutorials()
{
	if (IsDevMode()) {
		if (shownBoostTutorials.empty()) {
			// пометим все туториалы как пройденные
			std::vector<std::string> names;
			for (std::map<int, BoostTutorial>::iterator i = boostTutorials.begin(), e = boostTutorials.end(); i != e; ++i) {
				shownBoostTutorials.push_back(i->second.name);
			}
		} else {
			// пометим все туториалы как не пройденные
			shownBoostTutorials.clear();
			// сбросим подсказки после бустов
			gameInfo.setLocalBool("TutorialAfterBoostThreeMoreMoves", false);
			gameInfo.setLocalBool("TutorialAfterBoostSuperChip", false);
			gameInfo.setLocalBool("TutorialAfterBoostClearColorBeforeGame", false);
		}
	}
}

GameInfo gameInfo;
