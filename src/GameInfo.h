#ifndef GAME_INFO_H_INCLUDED
#define GAME_INFO_H_INCLUDED

#include "CommonContainers.h"
#include "IGameInfo.h"
#include "DataStoreWithRapid.h"

namespace GUI {
	class Widget;
}
namespace Render {
	class Text;
}
class LiveCounter;
class Message;
class UserAvatar;
struct FriendInfo;
class TText;

class GameInfo
	: public IGameInfo
{
private:
	struct PlayerInfo
		: public DataStoreWithRapid
	{
		PlayerInfo() {};
	};

	// информация о сохранении игрока
	PlayerInfo playerInfo;

	virtual std::string getLocalProperty(const std::string &name, const std::string &def = "");
	virtual void setLocalProperty(const std::string &name, const std::string &value);

public:

	GameInfo();
	~GameInfo();

	// режим разработчика (локальная переменная "user_type" должна быть равна 1)
	bool IsDevMode();

	bool localNameDefined(const std::string &name);

	int getLocalInt(const std::string& name, int defaultValue = 0);
	bool getLocalBool(const std::string& name, bool defaultValue = false);
	float getLocalFloat(const std::string& name, float defaultValue = 0.0f);
	double getLocalDouble(const std::string& name, double defaultValue = 0.0f);
	std::string getLocalString(const std::string& name, std::string defaultValue = "");

	void setLocalInt(const std::string& name, int value);
	void setLocalBool(const std::string& name, bool value);
	void setLocalFloat(const std::string& name, float value);
	void setLocalDouble(const std::string& name, double value);
	void setLocalString(const std::string& name, const std::string& value);
	
	int getLocalArrInt(const std::string& name, int index);
	void setLocalArrInt(const std::string& name, int index, int value);

	void eraseLocalVariable(const std::string& name);

	void Init(); // загрузка констант из GameSettings.xml
	void ReloadConstants(); //Загрузка/перезагрузка констант
	void LoadFromFile(bool newSave = false); // загрузка GameInfo.xml (или GameInfoDefault.xml)
	void LoadDefaultVariables(); // выставление дефолтных значений некоторых переменных
	void Save(bool parseSave = true); // сохранение файла на диск, и каждые N-минут, сохранение в Parse (если parseSave == true)
	void parseSaved(); // парс сохранился сам, не нужно больше сохранять по времени

	// настройка звука и музыки volume - [0.0, 1.0]
	void setMusicVolume(float volume);
	void setSoundVolume(float volume);
	float getMusicVolume();
	float getSoundVolume();

private:
	void LoadFromXml(rapidxml::xml_node<>* root); // загрузка из xml

public:

	float GetTimeText(Render::Text* text);
	float GetTimeText(const std::string &text_id);
	// время прошедшее с начала эпохи
	double getGlobalTime();
	// обновление
	void Update(float dt);
	// cчетчик жизней
	LiveCounter* getLiveCounter() { return _liveCounter; }
	// аватарка игрока (с фейсбука или дефолтная)
	UserAvatar* getAvatar();
private:
	LiveCounter* _liveCounter;
	UserAvatar* avatar;
	double timeSaveToParse;

	int lastChangeCashValue;
	std::string lastChangeCashId;
	typedef boost::function< void() > ChangeCashHandler;
	ChangeCashHandler lastChangeCashCallback;
public:
	
	struct PurchaseHistory {
		int value;
		std::string purchaseId;
		double time;
		PurchaseHistory(int value, const std::string& purchaseId, double time)
			:value(value), purchaseId(purchaseId), time(time) {};
	};
	// история покупок виртуальных действий за виртуальные деньги (покупка жизней, покупка ключей, покупка бустов)
	std::list<PurchaseHistory> purchaseHistory;

	struct PurchaseCashHistory {
		int value;
		bool realCash;
		double time;
		PurchaseCashHistory(int value, bool realCash, double time)
			: value(value), realCash(realCash), time(time) {};
	};
	// история получения виртуальных денег
	std::list<PurchaseCashHistory> purchaseCashHistory;

	struct Price {
		Price() { Assert(false); }; // нужен для std::map, но не нужен для игры
		Price(std::string id, int cash): id(id), cash(cash) {};
		std::string id;
		int cash;
	};
	// цены на покупку кэша, задаются в GameSettings.xml
	std::map<std::string, Price> prices;
	// купить покупку с идентификатором productId
	void buy(std::string productId);

	// количество кэша у игрока (free_cash + real_cash)
	int getCash();
	// потратить кэш (сначала тратиться free_cash, потом real_cash)
	// следует проверять, хватит ли кэша, иначе будет ассерт
	// value - сколько тратиться денег
	// callback - вызывается при ответе от сервера о удачной трате денег
	// purchaseId - идентификатор покупки для хранения в истории покупок и статистики
	void spendCash(int value, ChangeCashHandler callback, const std::string& purchaseId);
	// трата денег после ответа от сервера
	void _spendCash(bool successfulRefresh);
	
	// добавить кэш
	void addCash(int value, bool realCash, ChangeCashHandler callback = NULL);
	// добавить денег после ответа от сервера
	void _addCash(bool successfulRefresh);

	// Игровые данные друзей
	std::vector<FriendInfo*> friends; // Данные о друзьях.
	// Получить информацию о друге по фейсбук-идентификатору
	FriendInfo* getFriendByUid(const std::string facebookUid);

	// обновляет все локальные уведомления
	void updateLocalNotifications();
	// установить переменную, что играет, возможно мошенник
	void setPerhapsCheater();
	// uuid для swrve
	std::string GetUUID(unsigned int amount = 8);

	struct BoostTutorial
	{
		std::string name;
		int level;				// уровень на котором будет туториал
		int type;				// тип: 1 -перед уровнем, 2 -во время уровня
		std::vector<IPoint> points; //клетки поля (если нужны для туториала)
		BoostTutorial()
			:name(""), level(0), type(0) {};
		BoostTutorial(std::string name, int level, int type, std::vector<IPoint> points)
			:name(name), level(level), type(type),
			points(points) {};
	};
	BoostTutorial currentBoostTutorial;
	// туториалы о бустерах
	std::map<int, BoostTutorial> boostTutorials;
	// показанные туториалы о бустерах
	std::vector<std::string> shownBoostTutorials;
	// загрузить информацию о туториалах из xml
	void LoadBoostTutorials();
	// был ли показан туториал
	bool wasBoostTutorialShown(std::string name);
	// нужно ли показывать туториал
	bool needBoostTutorial(int level, int type);
	// доступен ли буст
	// (True если был показан или сейчас его туториал)
	bool allowBoostTutorial(std::string name, int level);
	// получить имя туториала
	std::string getBoostTutorialName();
	// считать туториал показанным
	void setBoostTutorialShown(std::string name);
	// запись пройденных туториалов в xml
	void SaveShownBoostTutorials(rapidxml::xml_node<>* elem);
	// чтение пройденных туториалов из xml
	void LoadShownBoostTutorials(rapidxml::xml_node<>* elem);
	// сбросить показанные туториалы
	void ResetShownBoostTutorials();
};

extern GameInfo gameInfo;

#endif //GAME_INFO_H_INCLUDED
