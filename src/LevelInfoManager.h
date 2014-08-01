#pragma once

#include "DataStoreWithRapid.h"

class LevelInfoManager {

private:
	// настройки уровней
	struct LevelInfo {
		std::string name;
		int number;
		DataStoreWithRapid data;
		void Load(rapidxml::xml_node<> *xml_level_info, int level_num);
	};
	std::vector<LevelInfo> _levelMap;
	// данные баланса уровней из Swrve
	typedef std::map<std::string, std::string> Dict;
	std::map<std::string, Dict> _levelSettingsAB;

public:
	LevelInfoManager();

	// загрузить краткое описание уровней
	void LoadLevelMap();
	// количество звезд на уровне
	int getLevelStars(int levelIndex);
	// количество очков на уровне
	int getLevelScore(int levelIndex);
	// обработка ресурсов, полученных из swrve
	void OnABInfo(const char *data);

	// следующие функции работают только с текущим уровнем (переменная "current_level" в gameInfo устанавливается во время клика по маркеру-уровня)

	// установить номер текущего уровня (плюс загрузить настройки уровня из файла уровня)
	void setCurrentLevel(int levelIndex);
	// сколько звезд при данном score заработает игрок на текущем уровне
	int getStarsForScore(int score);
	// сколько очков нужно заработать, чтобы заработать на звезду star
	int getScoreForStar(int star);
	// Является ли текущий уровень туториальным. В котором возможны некоторые поблажки.
	// (определяет отнимать ли жизнь при проигрыше/выходе/рестарте с текущего уровня)
	bool isTutorialLevel();
	// текущий уровень стартовал? (сделали первых ход)
	bool isLevelStarted();
	// Сквозная информация из LevelMap.xml об текущем уровне
	DataStoreWithRapid& getCurrentLevelData();
	// Текстовая информация о цели текущего уровня (например: провести энергию, набрать определенное количество очков)
	std::string getCurrentLevelGoal();
	size_t GetSize();

	// Индекc уровня в LevelMap по его имени
	int GetLevelIndex(const std::string& name);
	// название уровня
	std::string GetLevelName(int level = -1);
	// информация о уровне
	LevelInfo& GetLevelInfo(int level = -1);
	// изменить количество звезд на уровне (для дебага)
	void setLevelStars(int levelIndex, int count_stars);
	// установить полученные на уровне очки и звезды (устанавливает на текущий уровень)
	void setLevelScore(int score);

	Dict GetLevelSettingsAB(const std::string &levelName);
	// устанавливается в false при загрузке уровня, и в true при совершении первого хода
	bool levelStarted;
};

extern LevelInfoManager levelsInfo;