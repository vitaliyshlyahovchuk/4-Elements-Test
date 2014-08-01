#include "stdafx.h"
#include "ParseManager.h"
#include "parse_wrapper.h"
#include "MyEngineWrapper.h"
#include "FBInterface.h"
#include "GameInfo.h"
#include "SwrveManager.h"
#include "LevelInfoManager.h"
#include "LiveCounter.h"
#include "FriendInfo.h"
#include "Core.h"
#include "Utils/Str.h"
#include "AppCommon.h"
#include <json/json.h>

std::string CLASS_TABLE = "Info"; // таблица в parse базе данных, куду ведется запись
std::string facebookId = ""; // идентификатор фейсбука
std::string facebookName = "";
enum WAIT_PARSE_STATE {
	READY = 0, // ничиего не ждем от парса
	FIRST_ANSWER, // ждем ответа первого запроса к парсу
	FACEBOOK_LOGIN, // ждем ответ от фейсбука при логине
	FACEBOOK_LOGOUT, // ждем ответ от фейсбука при логауте
	WAIT_SAVE, // ждем ответа от сохранения
	RECONNECT, // ждем ответ при рекконнекте
	UPDATE_CASH, // ждем ответ при обновлении кэша с сервера
} wait_state;
double wait_parse_time = 0.0; // время начала запроса к парсу (для реалзиации таймаута)
// сколько времени ждем ответа от первого запроса (в StartLoad)
#ifdef _DEBUG
const double START_TIMEOUT = 40.0; // в дебаге, в симуляторе, не успевает загружаться за 10 секунд
#else
const double START_TIMEOUT = 10.0;
#endif
const double NEXT_TIMEOUT = 16.0; // сколько времени ждем ответа от остальных запросов

bool needReconnect = false; // требуется переконнектится к парсу через определенное время
double needReconnectTime = 0.0; // время следующего переконнекта к парсу
int reconnectAttempt = 0;
double getReconnectTime() {
	if (reconnectAttempt == 0) return gameInfo.getGlobalTime() + 15.0;
	if (reconnectAttempt < 3) return gameInfo.getGlobalTime() + 30.0 * reconnectAttempt;
	return gameInfo.getGlobalTime() + 180.0;
}

// -------------------------------------
// Следующие данные должны быть защищены блокировкой.
boost::recursive_mutex parseMutex;
#define THREAD_LOCK boost::recursive_mutex::scoped_lock lock(parseMutex)

ParseWrapper::ObjectPtr currentInfo = NULL; // текущий объект сохранения
ParseWrapper::QueryPtr lastQuery = NULL; // последний выполненный запрос
bool needSpendCash = true; // при обновлении кэша, указывает что нужно делать при получении ответа (true - потратить кэш, false - добавить кэш)

void ParseFriendsLoaded();
void ParseEndLoad();
void ParseReload();
void deviceForSwrve(std::string& type, std::string& subtype);

namespace ParseManager {
	
	// forward declaration
	void syncGameInfo(bool reconnect); // синхронизирует данные с сервера (currentInfo) с локальными данными (GameInfo.xml)
	void reloadGameInfo(const std::string& replacements = "all"); // заменяет данные gameInfo (GameInfo.h) на данные из info (PFObject*)
	void mergeGameInfo(); // сливает серверное сохранение с локальным
	void EndLoad(); //
	ParseWrapper::ObjectPtr newSave(); //
	void GameDBInfoFromPFObject(ParseWrapper::ObjectPtr GameInfoData, FriendInfo &info);
	void LoadFriendsInfo();
		
	// упакованные в json строку переменные, которые должны синхронизироваться с сервером
	Json::Value getJSONData();
	// загрузить из json переменные, которые должны синхронизироваться с сервером
	void loadJSONData(Json::Value &value);

	// Инициализация
	void Init(const std::string& appId, const std::string& clientKey) {
		ParseWrapper::Init(appId, clientKey);
	}
		
	void StartLoadOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects);
		
	void StartLoad() {
		std::string uid = gameInfo.getLocalString("uid", "");
		if (uid == "") {
			uid = GetUniqueDeviceIdentifier();
			gameInfo.setLocalString("uid", uid);
		}
		// пробуем получить сохранку с сервера
		wait_state = FIRST_ANSWER;
		wait_parse_time = gameInfo.getGlobalTime() + START_TIMEOUT;
		ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
		query->WhereKeyEqualTo("uid", Json::Value(uid));
		query->FindObjects(&StartLoadOnFindComplete);
		lastQuery = query;
	}

	void StartLoadOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects) {
		if (wait_state == FIRST_ANSWER || wait_state == RECONNECT) {
			bool reconnect = wait_state == RECONNECT;
			wait_state = READY;
			if (errorCode != ParseWrapper::ErrorCode_NoError()) {
				if (!reconnect) {
					ParseEndLoad();
				}
				needReconnect = true;
				needReconnectTime = getReconnectTime();
				++reconnectAttempt;
				return;
			}
			reconnectAttempt = 0;
			if (objects.empty()) {
				// сохранение новое или кто-то удалили сохранение
				currentInfo = newSave();
				gameInfo.Save(false);
				Save();
				if (!reconnect) {
					ParseEndLoad();
				}
				SwrveManager::TrackEvent("FirstRun", "{\"bver\":\"" + GetAppVersion() + "\"}");
				std::string typeDevice;
				std::string subtypeDevice;
				deviceForSwrve(typeDevice, subtypeDevice);
				std::string payload2 = typeDevice;
				if (payload2 == "iPod touch") {
					payload2 = "iPhone";
				}
				
				SwrveManager::TrackEvent("UserInfo.Device",
					"{\"type\":\"" + typeDevice + "\"" +
					",\"" + payload2 + "\":\"" + subtypeDevice + "\"}");
				return;
			}
			if (objects.size() > 1) {
				Assert(false); // не должно такого случиться, т.к. uid уникален
			}
			currentInfo = objects[0];
			syncGameInfo(reconnect);
			return;
		}
	}

	void syncGameInfo(bool reconnect) {
		std::string replacements = currentInfo->GetString("replacements", "");
		if (replacements != "" && !reconnect) {
			// частичное изменение локального сохранения (для Customer Server Tools)
			reloadGameInfo(replacements);
			currentInfo->RemoveKey("replacements");
			gameInfo.Save(false);
			Save();
			ParseEndLoad();
			return;
		}
		// слияние локальных параметров с сохраненными на сервере параметрами - на случай игры с двух устройств
		std::string deviceId = gameInfo.getLocalString("lastDeviceId");
		std::string serverDeviceId = currentInfo->GetString("lastDeviceId");
		if (! (serverDeviceId == "" || deviceId != serverDeviceId)) {
			// сохранение на сервере происходило с данного устройства - синхронизация завершена
			if (!reconnect) {
				ParseEndLoad();
			} else {
				gameInfo.Save(false);
				Save();
			}
			return;
		} else {
			// сохранение происходило с другого устройства - нужно слить сохранения
			mergeGameInfo();
			gameInfo.Save(false);
			Save();
			if (!reconnect) {
				ParseEndLoad();
			} else {
				ParseReload();
			}
		}
	}
		
	void reloadGameInfo(const std::string& replacements) {
		
		const int INT_TYPE = 1; // int
		const int ARRAY_INT_TYPE = 2; // array(int)
		const int STRING_TYPE = 3; // string
		std::map<std::string, int> FieldsType;
		
		FieldsType["extreme_level"] = INT_TYPE;
		FieldsType["current_marker"] = INT_TYPE;
		FieldsType["free_cash"]  = INT_TYPE;
		FieldsType["real_cash"] = INT_TYPE;
		FieldsType["user_type"] = INT_TYPE;
		FieldsType["level_score"] = ARRAY_INT_TYPE;
		FieldsType["level_stars"] = ARRAY_INT_TYPE;
		FieldsType["fbId"] = STRING_TYPE;
		FieldsType["fbName"] = STRING_TYPE;
		FieldsType["lastFacebookId"] = STRING_TYPE;
		
		std::vector<std::string> fields;
		if (replacements == "all") {
			for (auto i = FieldsType.begin(), e = FieldsType.end(); i != e; ++i) {
				fields.push_back(i->first);
			}
			gameInfo.LoadDefaultVariables();
			Json::Value variables = currentInfo->GetValue("vars");
			if (variables.isNull()) {
				// в старой версии в variables хранилась строка, содеражащая json объект
				//todo: потом убрать всю эту ветку
				variables = currentInfo->GetValue("variables");
				if (!variables.isNull()) {
					Json::Reader reader;
					reader.parse(variables.asString(), variables);
				}
			}
			loadJSONData(variables);
			SwrveManager::Start();
		} else {
			fields = utils::String::Split(replacements, ',', true);
		}
		if (std::find(fields.begin(), fields.end(), "ResetGame") != fields.end()) {
			// сброс сохранения из customers tools
			std::string uid = gameInfo.getLocalString("uid");
			std::string fbId = gameInfo.getLocalString("fbId");
			std::string fbName = gameInfo.getLocalString("fbName");
			gameInfo.LoadFromFile(true);
			gameInfo.setLocalString("uid", uid);
			gameInfo.setLocalString("fbId", fbId);
			gameInfo.setLocalString("fbName", fbName);
			currentInfo->DeleteBlocking();
			currentInfo = newSave();
			return;
		}
			
		for (size_t i = 0; i < fields.size(); ++i) {
			std::string& field = fields[i];
			Json::Value value = currentInfo->GetValue(field);
			if (!value.isNull()) {
				switch (FieldsType[field]) {
					case INT_TYPE:
						gameInfo.setLocalInt(field, value.asInt());
						break;
					case STRING_TYPE:
						gameInfo.setLocalString(field, value.asString());
						break;
					case ARRAY_INT_TYPE:
						{
							Assert(value.isArray());
							gameInfo.eraseLocalVariable(field);
							for (size_t i = 0, size = value.size(); i < size; ++i) {
								gameInfo.setLocalArrInt(field, i, value[static_cast<int>(i)].asInt());
							}
						}
						break;
					default:
						Assert(false);
				}
			}
		}

		// туториалы про бусты
		Json::Value value = currentInfo->GetValue("shownBoostTutorials");
		gameInfo.shownBoostTutorials.clear();
		if (!value.isNull()) {
			for (size_t i = 0, size = value.size(); i < size; ++i) {
				gameInfo.shownBoostTutorials.push_back(value[static_cast<int>(i)].asString());
			}
		}
		
		// перегрузка жизней (возможно они изменились)
		if (gameInfo.getLiveCounter()) {
			gameInfo.getLiveCounter()->load();
		}
	}
		
	void mergeGameInfo() {
		
		const int INT_TYPE = 1; // int
		const int INT_TYPE_MAX = 2; // int - max
		const int ARRAY_INT_TYPE_MAX = 3; // array(int) - max
		const int STRING_TYPE = 4; // string
		
		std::map<std::string, int> FieldsType;
		
		FieldsType["extreme_level"] = INT_TYPE_MAX;
		FieldsType["current_marker"] = INT_TYPE_MAX;
		FieldsType["free_cash"]  = INT_TYPE;
		FieldsType["real_cash"] = INT_TYPE;
		FieldsType["user_type"] = INT_TYPE;
		FieldsType["lastFacebookId"] = STRING_TYPE;
		FieldsType["level_score"] = ARRAY_INT_TYPE_MAX;
		FieldsType["level_stars"] = ARRAY_INT_TYPE_MAX;
		
		std::vector<std::string> fields;
		for (auto i = FieldsType.begin(), e = FieldsType.end(); i != e; ++i) {
			fields.push_back(i->first);
		}
		for (size_t i = 0; i < fields.size(); ++i) {
			std::string& field = fields[i];
			Json::Value value = currentInfo->GetValue(field);
			if (!value.isNull()) {
				switch (FieldsType[field]) {
					case INT_TYPE:
						gameInfo.setLocalInt(field, value.asInt());
						break;
					case STRING_TYPE:
						gameInfo.setLocalString(field, value.asString());
						break;
					case INT_TYPE_MAX:
						gameInfo.setLocalInt(field, std::max(gameInfo.getLocalInt(field), value.asInt()));
						break;
					case ARRAY_INT_TYPE_MAX:
						{
							Assert(value.isArray());
							for (size_t i = 0, size = value.size(); i < size; ++i) {
								int server_value = value[static_cast<int>(i)].asInt();
								int client_value = gameInfo.getLocalArrInt(field, i);
								gameInfo.setLocalArrInt(field, i, std::max(server_value, client_value));
							}
						}
						break;
					default:
						Assert(false);
				}
			}
		}
		// туториалы про бусты
		Json::Value value = currentInfo->GetValue("shownBoostTutorials");
		if (!value.isNull()) {
			for (size_t i = 0, size = value.size(); i < size; ++i) {
				std::string name = value[static_cast<int>(i)].asString();
				// если такого туториала ещё нет, запишем
				if (!gameInfo.wasBoostTutorialShown(name)) {
					gameInfo.shownBoostTutorials.push_back(name);
				}
			}
		}
		
		Json::Value variables = currentInfo->GetValue("vars");
		if (variables.isNull()) {
			// в старой версии в variables хранилась строка, содеражащая json объект
			variables = currentInfo->GetValue("variables");
			if (!variables.isNull()) {
				Json::Reader reader;
				reader.parse(variables.asString(), variables);
			}
		}
		loadJSONData(variables);
	}

	ParseWrapper::ObjectPtr newSave() {
		return ParseWrapper::Object::CreateObject(CLASS_TABLE);
	}
		
	void loginOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects);
	void logoutOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects);

	std::string tempFbId;
	std::string tempFbName;
	bool needRegisterFaceboolId = false;
	void RegisterFacebookID(const std::string& fbId, const std::string& fbName)
	{
		if (wait_state == FIRST_ANSWER) {
			// заняты первым запросом к парсу, подождем его ответа и выполним RegisterFacebookID(...) снова
			tempFbId = fbId;
			tempFbName = fbName;
			needRegisterFaceboolId = true;
			return;
		}
		Log::Info("Faceebook answer: fbId: " + fbId + " fbName: (" + fbName + ")");
		if (fbId != "") {
			facebookId = fbId;
			facebookName = fbName;
			if (facebookId == gameInfo.getLocalString("uid")) {
				// сохранка загружена правильная, повторный запрос не требуется
				return;
			}
			wait_state = FACEBOOK_LOGIN;
			wait_parse_time = gameInfo.getGlobalTime() + NEXT_TIMEOUT;
			ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
			query->WhereKeyEqualTo("uid", Json::Value(facebookId));
			query->FindObjects(&loginOnFindComplete);
			lastQuery = query;
		} else {
			if (facebookId != "") {
				// происходит logout
				wait_state = FACEBOOK_LOGOUT;
				wait_parse_time = gameInfo.getGlobalTime() + NEXT_TIMEOUT;
				gameInfo.setLocalString("lastFacebookId", facebookId);
				facebookId = "";
				facebookName = "";
				std::string uid = GetUniqueDeviceIdentifier();
				gameInfo.setLocalString("uid", uid);
				gameInfo.Save(false);
				currentInfo = NULL;
				ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
				query->WhereKeyEqualTo("uid", Json::Value(uid));
				query->FindObjects(&logoutOnFindComplete);
				lastQuery = query;
			}
		}
	}

	void loginOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects) {
		if (wait_state == FACEBOOK_LOGIN) {
			wait_state = READY;

			gameInfo.setLocalString("uid", facebookId);
			gameInfo.setLocalString("fbId", facebookId);
			gameInfo.setLocalString("fbName", facebookName);
			if (errorCode != ParseWrapper::ErrorCode_NoError()) {
				// возможно - пропал интернет
				gameInfo.Save(false);
				currentInfo = NULL;
				needReconnect = true;
				needReconnectTime = getReconnectTime();
				++reconnectAttempt;
				return;
			}
			reconnectAttempt = 0;
			
			if (objects.empty()) {
				// Сохранка не найдена
				if (gameInfo.getLocalString("lastFacebookId") != "") {
					// другой аккаунт уже играл на данном устройстве, значит создаем новое сохранение
					gameInfo.LoadFromFile(true);
					gameInfo.setLocalString("uid", facebookId);
					gameInfo.setLocalString("fbId", facebookId);
					gameInfo.setLocalString("fbName", facebookName);
					Core::LuaCallVoidFunction("ResetGameOnFacebookLogin");
				}
				currentInfo = newSave();
				// добавляем кэша за коннект к фейсбуку (без проверок, т.к. currentInfo еще не существуюет на сервере)
				gameInfo.setLocalInt("free_cash", gameInfo.getLocalInt("free_cash") + gameInfo.getConstInt("FBConnectAward"));
				// событие о выдаче валюты
				SwrveManager::TrackPurchase("currency_given.Facebook", gameInfo.getConstInt("FBConnectAward"), SwrveManager::ALL_CURRENCY);
				// событие о первом коннекте (сделано в lua, т.к. надо знать, через какую кнопку пошли логиниться)
				Core::LuaCallVoidFunction("FacebookFirstTimeConnected");
				// сохраняем GameInfo.xml
				gameInfo.Save(false);
				// отправляем на сервер
				Save(true);
				return;
			}
			if (objects.size() > 1) {
				Assert(false); // не должно такого случиться, т.к. facebookId должен сохраниться только один раз
			}
			currentInfo = objects[0];
			if (gameInfo.getLocalString("lastFacebookId") == "" || facebookId != gameInfo.getLocalString("lastFacebookId")) {
				// вошли в фейсбук другим аккаунтом -
				// начинаем играть в полученную сохранку
				reloadGameInfo();
				gameInfo.Save(false);
				// отправляем на сервер
				Save();
				ParseReload();
				return;
			} else {
				// вошли в фейсбук тем же аккаунтом
				// посмотрим, с какого устройства последний раз сохранялись
				std::string deviceId = currentInfo->GetString("lastDeviceId");
				if (deviceId != gameInfo.getLocalString("lastDeviceId")) {
					// если  последнее сохранение было с другого устройства - сливаем
					mergeGameInfo();
					ParseReload();
					return;
				} else {
					// последнее сохранение происходило с данного устройства
					// ничего синхронизировать не нужно
					gameInfo.Save(false);
				}
			}
		}
	}
		
	void logoutOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects) {
		if (wait_state == FACEBOOK_LOGOUT) {
			wait_state = READY;
			if (errorCode != ParseWrapper::ErrorCode_NoError()) {
				needReconnect = true;
				needReconnectTime = getReconnectTime();
				++reconnectAttempt;
				return;
			}
			reconnectAttempt = 0;
			
			if ( objects.empty()) {
				// кто-то удалил сохранку?
				currentInfo = newSave();
				gameInfo.Save(false);
				Save();
				return;
			}
			if (objects.size() > 1) {
				Assert(false); // не должно такого случиться, т.к. uid уникален
			}
			currentInfo = objects[0];
			Save();
		}
	}

	void onSaveComplete(int errorCode, ParseWrapper::ObjectOperationType::Type type) {
		if (type == ParseWrapper::ObjectOperationType::SAVE) {
			if (errorCode == ParseWrapper::ErrorCode_NoError() ) {
				if (wait_state == WAIT_SAVE) {
					wait_state = READY;
				}
				Log::Info("parse saved");
				reconnectAttempt = 0;
			} else {
				if (!needReconnect) {
					currentInfo = NULL;
					needReconnect = true;
					needReconnectTime = getReconnectTime();
					++reconnectAttempt;
				}
			}
		}
	}
	
	// Сохранить данные из gameInfo
	void Save(bool withMoney) {
		if (currentInfo) {
			Log::Info("parse start save");
			// уникальный идентификатор
			currentInfo->SetString("uid", gameInfo.getLocalString("uid"));
			// уровень
			int extreme_level = gameInfo.getLocalInt("extreme_level");
			currentInfo->SetInt("extreme_level", extreme_level);
			// продвижение по карте
			currentInfo->SetInt("current_marker", gameInfo.getLocalInt("current_marker"));
			// очки на уровнях
			Json::Value value(Json::arrayValue);
			for (int i = 0; i < extreme_level; ++i) {
				value.append(Json::Value(levelsInfo.getLevelScore(i)));
			}
			currentInfo->SetValue("level_score", value);
			// звезды на уровнях
			value.clear();
			for (int i = 0; i < extreme_level; ++i) {
				value.append(Json::Value(levelsInfo.getLevelStars(i)));
			}
			currentInfo->SetValue("level_stars", value);
			// игровая валюта
			if (withMoney) {
				currentInfo->SetInt("free_cash", gameInfo.getLocalInt("free_cash"));
				currentInfo->SetInt("real_cash", gameInfo.getLocalInt("real_cash"));
			}
			
			// идентификатор фейсбука и имя
			if (facebookId == gameInfo.getLocalString("uid")) {
				currentInfo->SetString("fbId", gameInfo.getLocalString("fbId"));
				currentInfo->SetString("fbName", gameInfo.getLocalString("fbName"));
			}
			if (gameInfo.getLocalString("lastFacebookId") != "") {
				currentInfo->SetString("lastFacebookId", gameInfo.getLocalString("lastFacebookId"));
			}
			
			// является ли игрок читером / разработчиком
			if (gameInfo.getLocalInt("user_type") != 0) {
				currentInfo->SetInt("user_type", gameInfo.getLocalInt("user_type"));
			}

			currentInfo->SetString("lastDeviceId", GetUniqueDeviceIdentifier());
			
			// покупки игровых сущностей
			value = currentInfo->GetValue("purchaseHistory", ParseWrapper::ValueType::ARRAY);
			if (value.isNull()) {
				value = Json::Value(Json::arrayValue);
			}
			std::string deviceId = GetDeviceIdName();
				
			bool need_update = false;
			for (auto i = gameInfo.purchaseHistory.begin(), e = gameInfo.purchaseHistory.end(); i != e; ++i) {
				Json::Value item(Json::objectValue);
				item["value"] = Json::Value(i->value);
				item["purchaseId"] = Json::Value(i->purchaseId);
				item["deviceId"] = Json::Value(deviceId);
				item["time"] = Json::Value(i->time);
				value.append(item);
				need_update = true;
			}
			if (need_update) {
				while (value.size() > 512) {
					// в поле Object может хранится до 128килобайт
					// на одну запись purchaseHistory выделяем 256 байт
					//todo: как удалить первый элемент?
				}
				currentInfo->SetValue("purchaseHistory", value);
				gameInfo.purchaseHistory.clear();
			}
			// покупка кэша в app store
			value = currentInfo->GetValue("purchaseCashHistory");
			if (value.isNull()) {
				value = Json::Value(Json::arrayValue);
			}
			need_update = false;
			for (auto i = gameInfo.purchaseCashHistory.begin(), e = gameInfo.purchaseCashHistory.end(); i != e; ++i) {
				Json::Value item(Json::objectValue);
				item["value"] = Json::Value(i->value);
				item["deviceId"] = Json::Value(deviceId);
				item["realCash"] = Json::Value(i->realCash);
				item["time"] = Json::Value(i->time);
				value.append(item);
				need_update = true;
			}
			if (need_update) {
				while (value.size() > 512) {
					// в поле PFObject может хранится до 128килобайт
					// на одну запись purchaseHistory выделяем 256 байт
					//todo: как удалить первый элемент?
				}
				currentInfo->SetValue("purchaseCashHistory", value);
				gameInfo.purchaseCashHistory.clear();
			}
			// туториалы про бусты
			value = Json::Value(Json::arrayValue);
			for (auto i = gameInfo.shownBoostTutorials.begin(), e = gameInfo.shownBoostTutorials.end(); i != e; ++i) {
				value.append(Json::Value(*i));
			}
			currentInfo->SetValue("shownBoostTutorials", value);
			
			// различне переменные, которые должны хранится на сервере
			currentInfo->SetValue("vars", getJSONData());

			// Сохраняемся.
			currentInfo->Save(&onSaveComplete);
			wait_state = WAIT_SAVE;
			wait_parse_time = gameInfo.getGlobalTime() + NEXT_TIMEOUT;
			gameInfo.parseSaved();
		}
	}


	void GameDBInfoFromPFObject(ParseWrapper::ObjectPtr object, FriendInfo &info)
	{
		// сделано через AllKeys, так как ContainsKey падает в ios, если там nil
		std::vector<std::string> allKeys = object->AllKeys();
		std::map<std::string, bool> keys;
		for (size_t i = 0, size = allKeys.size(); i < size; ++i) {
			keys[allKeys[i]] = true;
		};
		std::map<std::string, bool>::iterator keysEnd = keys.end();
		
		if (keys.find("fbId") != keysEnd) {
			info.fb_id = object->GetString("fbId");
		}
		
		if (keys.find("fbName") != keysEnd) {
			info.name = object->GetString("fbName");
		}
		
		if (keys.find("extreme_level") != keysEnd) {
			info.extreme_level = object->GetInt("extreme_level");
		}
		
		if (keys.find("current_marker") != keysEnd) {
			info.current_marker = object->GetInt("current_marker");
		}
		
		if (keys.find("level_score") != keysEnd) {
			Json::Value array = object->GetValue("level_score");
			if (!array.isNull()) {
				for (size_t i = 0, size = array.size(); i < size; ++i) {
					info.level_scores.push_back(array[static_cast<int>(i)].asInt());
				}
			}
		}
	}
		
	void loadFriendsInfoOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects);

	// Загружает информацию по друзьям
	void LoadFriendsInfo()
	{
		std::vector<std::string> fbIds = FB::FriendIds();
		if (fbIds.empty()) { return; }

		Log::Info("Parse: LoadFriendsInfo");

		// Запрашиваем инфу по всем друзьям.
		ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
		std::vector<std::string> keys;
		keys.push_back("fbId");
		keys.push_back("fbName");
		keys.push_back("extreme_level");
		keys.push_back("current_marker");
		keys.push_back("level_score");
		query->SelectKeys(keys);
		Json::Value jsIds(Json::arrayValue);
		for (size_t i = 0; i < fbIds.size(); ++i) {
			jsIds.append(Json::Value(fbIds[i]));
		}
		query->WhereKeyContainedIn("uid", jsIds);
		query->SetLimit(512);
		query->FindObjects(&loadFriendsInfoOnFindComplete);
	}

	void loadFriendsInfoOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects) {
		if (errorCode == ParseWrapper::ErrorCode_NoError()) {
			THREAD_LOCK;
			for (size_t i = 0, size = objects.size(); i < size; i++) {
				FriendInfo *info = new FriendInfo();
				GameDBInfoFromPFObject(objects[i], *info);
				gameInfo.friends.push_back(info);
			}
			ParseFriendsLoaded();
		}
	}

	void ResetOnFindComplete(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects) {
		if (errorCode == ParseWrapper::ErrorCode_NoError()) {
			for (size_t i = 0, size = objects.size(); i < size; i++) {
				objects[i]->DeleteBlocking();
			}
		}
	}
	// дебажный сброс сохранения - синхронно удаляем сохранку с сервера
	void Reset() {
		if (currentInfo) {
			currentInfo = NULL;
		}
		// удаляем фейсбук сохранение или сохранение, привязанное к устройству
		std::string uid = gameInfo.getLocalString("uid", "");
		if (uid != "") {
			ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
			query->WhereKeyEqualTo("uid", Json::Value(uid.c_str()));
			query->FindObjectsBlocking(&ResetOnFindComplete);
		}
		// теперь удалим сохранение, привязанное к устройству
		ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
		query->WhereKeyEqualTo("uid", Json::Value(GetUniqueDeviceIdentifier().c_str()));
		query->FindObjectsBlocking(&ResetOnFindComplete);
	}
		

	Json::Value getJSONData() {
		
		const int BOOL_TYPE = 1;
		const int INT_TYPE = 2;
		const int STRING_TYPE = 3;
		
		std::map<std::string, int> FieldsType;
		std::string str_params;
		std::vector<std::string> params;
		str_params = gameInfo.getConstString("ServerBoolParams");
		if (!str_params.empty()) {
			params = utils::String::Split(str_params, ',');
			for (auto i = params.begin(), e = params.end(); i != e; ++i) {
				FieldsType[*i] = BOOL_TYPE;
			}
		}
		str_params = gameInfo.getConstString("ServerIntParams");
		if (!str_params.empty()) {
			params = utils::String::Split(str_params, ',');
			for (auto i = params.begin(), e = params.end(); i != e; ++i) {
				FieldsType[*i] = INT_TYPE;
			}
		}
		str_params = gameInfo.getConstString("ServerStringParams");
		if (!str_params.empty()) {
			params = utils::String::Split(str_params, ',');
			for (auto i = params.begin(), e = params.end(); i != e; ++i) {
				FieldsType[*i] = STRING_TYPE;
			}
		}

		Json::Value result(Json::objectValue);
		for (auto i = FieldsType.begin(), e = FieldsType.end(); i != e; ++i) {
			switch (i->second) {
				case BOOL_TYPE:
					result[i->first] = gameInfo.getLocalBool(i->first);
					break;
				case INT_TYPE:
					result[i->first] = gameInfo.getLocalInt(i->first);
					break;
				case STRING_TYPE:
					result[i->first] = gameInfo.getLocalString(i->first);
					break;
				default:
					Assert(false);
					break;
			}
		}
		return result;
	}

	void loadJSONData(Json::Value& data) {
		
		const int BOOL_TYPE = 1;
		const int INT_TYPE = 2;
		const int STRING_TYPE = 3;
		
		std::map<std::string, int> FieldsType;
		std::string str_params;
		std::vector<std::string> params;
		str_params = gameInfo.getConstString("ServerBoolParams");
		if (!str_params.empty()) {
			params = utils::String::Split(str_params, ',');
			for (auto i = params.begin(), e = params.end(); i != e; ++i) {
				FieldsType[*i] = BOOL_TYPE;
			}
		}
		str_params = gameInfo.getConstString("ServerIntParams");
		if (!str_params.empty()) {
			params = utils::String::Split(str_params, ',');
			for (auto i = params.begin(), e = params.end(); i != e; ++i) {
				FieldsType[*i] = INT_TYPE;
			}
		}
		str_params = gameInfo.getConstString("ServerStringParams");
		if (!str_params.empty()) {
			params = utils::String::Split(str_params, ',');
			for (auto i = params.begin(), e = params.end(); i != e; ++i) {
				FieldsType[*i] = STRING_TYPE;
			}
		}
		
		auto keys = data.getMemberNames();
		for (auto i = keys.begin(), e = keys.end(); i != e; ++i) {
			switch (FieldsType[*i]) {
				case BOOL_TYPE:
					gameInfo.setLocalBool(*i, data[*i].asBool());
					break;
				case INT_TYPE:
					gameInfo.setLocalInt(*i, data[*i].asInt());
					break;
				case STRING_TYPE:
					gameInfo.setLocalString(*i, data[*i].asString());
					break;
				default:
					// вероятно, такая переменная больше не нужна на сервере
					break;
			}
		}
	}

	void Update(float dt) {
		// одновременный первый запрос к парсу и коннект к фейсбуку
		if (needRegisterFaceboolId && wait_state == READY) {
			needRegisterFaceboolId = false;
			RegisterFacebookID(tempFbId, tempFbName);
		}
		// таймаут для запросов к парсу
		if (wait_state != READY && wait_parse_time < gameInfo.getGlobalTime()) {
			if (lastQuery) {
				lastQuery->Cancel();
				lastQuery = NULL;
			}
			needReconnect = true;
			needReconnectTime = getReconnectTime();
			++reconnectAttempt;
			switch (wait_state) {
				case FIRST_ANSWER:
					ParseEndLoad();
					break;
				case FACEBOOK_LOGIN:
					gameInfo.setLocalString("uid", facebookId);
					gameInfo.setLocalString("fbId", facebookId);
					gameInfo.setLocalString("fbName", facebookName);
					gameInfo.Save(false);
					currentInfo = NULL;
					break;
				case FACEBOOK_LOGOUT:
					currentInfo = NULL;
					break;
				case WAIT_SAVE:
					currentInfo = NULL;
					break;
				case RECONNECT:
					break;
				case UPDATE_CASH:
					if (needSpendCash) {
						gameInfo._spendCash(false);
					} else {
						gameInfo._addCash(false);
					}
					break;
				default:
					Assert(false);
					break;
			}
			wait_state = READY;
		} else {
			// реконнект к парсу
			if (needReconnect) {
				if (needReconnectTime < gameInfo.getGlobalTime()) {
					needReconnect = false;
					wait_state = RECONNECT;
					wait_parse_time = gameInfo.getGlobalTime() + NEXT_TIMEOUT;
					std::string uid = gameInfo.getLocalString("uid", "");
					ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
					query->WhereKeyEqualTo("uid", Json::Value(uid));
					query->FindObjects(&StartLoadOnFindComplete);
					lastQuery = query;
				}
			}
		}
	}
		
	void onCompleteRefreshCash(int error_code, ParseWrapper::ObjectOperationType::Type type = ParseWrapper::ObjectOperationType::REFRESH) {
		if (wait_state == UPDATE_CASH) {
			wait_state = READY;
			if (error_code == ParseWrapper::ErrorCode_NoError()) {
				reconnectAttempt = 0;
				// обновим деньги
				gameInfo.setLocalInt("real_cash", currentInfo->GetInt("real_cash"));
				gameInfo.setLocalInt("free_cash", currentInfo->GetInt("free_cash"));
				if (needSpendCash) {
					gameInfo._spendCash(true);
				} else {
					gameInfo._addCash(true);
				}
				gameInfo.Save(false);
				Save(true);
				return;
			} else {
				if (needSpendCash) {
					gameInfo._spendCash(false);
				} else {
					gameInfo._addCash(false);
				}
			}
		}
	}
	
	void LoadInfoAndUpdateCash(int errorCode, const std::vector<ParseWrapper::ObjectPtr> &objects) {
		if (wait_state == UPDATE_CASH) {
			if (errorCode != ParseWrapper::ErrorCode_NoError()) {
				wait_state = READY;
				needReconnect = true;
				needReconnectTime = getReconnectTime();
				++reconnectAttempt;
				if (needSpendCash) {
					gameInfo._spendCash(false);
				} else {
					gameInfo._addCash(false);
				}
				return;
			}
			reconnectAttempt = 0;
			needReconnect = false;
			if (objects.empty()) {
				currentInfo = newSave();
				onCompleteRefreshCash(ParseWrapper::ErrorCode_NoError());
				return;
			}
			currentInfo = objects[0];
			gameInfo.setLocalInt("real_cash", currentInfo->GetInt("real_cash"));
			gameInfo.setLocalInt("free_cash", currentInfo->GetInt("free_cash"));
			if (needSpendCash) {
				gameInfo._spendCash(true);
			} else {
				gameInfo._addCash(true);
			}
			syncGameInfo(true);
		}
	}
	
	void RefreshCash(bool spendCash) {
		needSpendCash = spendCash;
		needReconnect = false;
		wait_state = UPDATE_CASH;
		wait_parse_time = gameInfo.getGlobalTime() + 5.0;
		
		if (currentInfo) {
			currentInfo->Refresh(&onCompleteRefreshCash);
		} else {
			std::string uid = gameInfo.getLocalString("uid", "");
			ParseWrapper::QueryPtr query = ParseWrapper::Query::GetSimpleQuery(CLASS_TABLE);
			query->WhereKeyEqualTo("uid", Json::Value(uid));
			query->FindObjects(&LoadInfoAndUpdateCash);
			lastQuery = query;
		}
	}

} // end of namespace
