#pragma once

#include "ChipsInfo.h"
#include "GroundInfo.h"
#include "UndergoundPrizes.h"
#include "ChipSource.h"
#include "DataStoreWithRapid.h"

class GameField;

namespace Gadgets
{
	extern const int dx8ccw[8];
	extern const int dy8ccw[8];

	struct CheckDirsInfo
	{
		const int *dx;
		const int *dy;
		size_t count;	
		IPoint operator [](const size_t i)
		{
			return IPoint(dx[i], dy[i]);
		};
	};

	extern CheckDirsInfo checkDirsInfo, checkDirsChains;

	//Сообщение с информацией об бустах для уровня
	extern Message message_boost_for_level;

	//Сообщение с информацие об бустах для магазина бустов в уровне
	extern Message message_boost_for_shop;

	extern DataStoreWithRapid levelSettings, levelSettingsDefault;

	extern std::vector<BaseEditorMaker*> editor_makers;				


	//Общее количество приемников энергии (в отличии от EnergyReceivers receivers загружается в LoadLevelSettings)
	extern int totalReceivers;

	//Информация о фишках (прикрепленные ходы, звездочки
	extern ChipsInfo chips_info;

	//Менеджер земли
	extern GroundMaker ground_maker;

	//Подземные бонусы
	extern UgPriseMaker ug_prises;

	extern ChipSources chipSources;


	void Init(GameField *game_field, bool with_editor);
	void Release();
	void Clear();
	void Save(Xml::TiXmlElement *xml_level);
	void LoadLevelSettings(rapidxml::xml_node<> *xml_level);
	void LoadLevel(rapidxml::xml_node<> *xml_level);
	void LoadWallTypes(rapidxml::xml_node<> *xml_level);
	void DrawEdit();
	
	// Функция удаляет какой-либо внешний объект c поля. То еcть такой, который не задаётcя
	// параметрами в Game::Square, а подcоединяетcя извне. Любая миниигра будет удалена этой функцией
	bool Editor_RemoveMinigameObject(Game::Square *cell, int button, bool totalRemove = true);

	//Событие: нажали на кнопку в редакторе (тут можно обнулять выделенные объекты например)
	void OnPushEditorButton();

	int LoadLevelDescription();
};