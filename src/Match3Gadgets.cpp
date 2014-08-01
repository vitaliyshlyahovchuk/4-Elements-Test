#include "stdafx.h"
#include "Match3Gadgets.h"
#include "EditorUtils.h"
#include "GameField.h"
#include "ActCounter.h"
#include "GameInfo.h"
#include "LevelInfoManager.h"
#include "FieldBears.h"
#include "SnapGadgetsClass.h"
#include "CellWalls.h"
#include "EnergyReceivers.h"
#include "LockBarriers.h"
#include "MovingMonster.h"
#include "ChangeEnergySpeedClass.h"
#include "BombField.h"
#include "RyushkiGadget.h"
#include "SquareNewInfo.h"
#include "RoomGates.h"

namespace Gadgets
{
	std::vector<BaseEditorMaker*> editor_makers;								

	//Общее количество приемников энергии
	int totalReceivers(0);

	//Информация о фишках (прикрепленные ходы, звездочки
	ChipsInfo chips_info;

	//Менеджер земли
	GroundMaker ground_maker;

	//Менеджер скрытых под стеной бонусов
	UgPriseMaker ug_prises;

	// Настраиваемые источники фишек (лакриц, тайм-бомб)
	ChipSources chipSources;

	//Настройки уровня
	DataStoreWithRapid levelSettings;
	DataStoreWithRapid levelSettingsDefault;

	CheckDirsInfo checkDirsInfo, checkDirsChains;


	void Init(GameField *game_field, bool with_editor)
	{
		editor_makers.clear();
		editor_makers.push_back(&ground_maker);
		editor_makers.push_back(&square_new_info);
		editor_makers.push_back(&chips_info);
		editor_makers.push_back(&receivers);
		editor_makers.push_back(&ug_prises);
		editor_makers.push_back(&lockBarriers);
		editor_makers.push_back(&chipSources);
		editor_makers.push_back(&cellWalls);
		editor_makers.push_back(&movingMonstersSources);
		editor_makers.push_back(&snapGadgets);
		editor_makers.push_back(&bears);
		editor_makers.push_back(&gates);

		for(size_t i = 0; i < editor_makers.size(); i++)
		{
			editor_makers[i]->Init(game_field, with_editor);
		}

		snapGadgets.Init(game_field);
		energySpeedChangers.Init(game_field);
	}

	void Release()
	{
		for(size_t i = 0; i < editor_makers.size(); i++)
		{
			editor_makers[i]->Release();
		}

		snapGadgets.Release();
		energySpeedChangers.Release();
	}

	void Clear()
	{
		for(size_t i = 0; i < editor_makers.size(); i++)
		{
			editor_makers[i]->Clear();
		}
		ryushki.ClearRyushki();
		snapGadgets.Clear();
		energySpeedChangers.Clear();
		bombFields.Clear();
	}

	void LoadWallTypes(rapidxml::xml_node<> *root)
	{
		std::vector< std::pair<Game::FieldAddress, std::string> > info;

		rapidxml::xml_node<> *xml_squares = root->first_node("WallTypesInfo");
		if(xml_squares)
		{
			rapidxml::xml_node<> *xml_square_info = xml_squares->first_node("square");
			while(xml_square_info)
			{
				std::string type = Xml::GetStringAttribute(xml_square_info, "type");
				info.push_back( std::make_pair(Game::FieldAddress(xml_square_info), type) );
				xml_square_info = xml_square_info->next_sibling("square");
			}
		}

		for (int i = 0; i < GameSettings::fieldWidth; i++){
			for (int j = 0; j < GameSettings::fieldHeight; j++){
				Game::Square *sq = GameSettings::gamefield[i+1][j+1];
				sq->SetSand(false, true);
				sq->SetIndestructible(false);
				sq->SetPermanent(false);
				sq->SetRestoring(false);
			}
		}

		for(std::vector< std::pair<Game::FieldAddress, std::string> >::iterator i = info.begin(); i != info.end(); )
		{
			Game::Square *sq = Game::GetValidSquare(i->first);
			//if( sq->GetWall() > 0) {
				if( i->second == "sand" )
					sq->SetSand(true, true);
				else if( i->second == "indestructible" )
					sq->SetIndestructible(true);
				else if( i->second == "permanent" )
					sq->SetPermanent(true);
				else if( i->second == "restoring" )
					sq->SetRestoring(true);
				else if( i->second.find("color") != std::string::npos && sq->GetWall() > 0)
					sq->SetWallColor( utils::lexical_cast<int>(i->second.substr(6)));
			//}
			i++;
		}
	}

	void LoadLevelSettings(rapidxml::xml_node<> *xml_level)
	{
		levelSettings = levelSettingsDefault;
		rapidxml::xml_node<> *xml_settings = xml_level->first_node("settings");
		if(xml_settings)
		{
			levelSettings.LoadAppend(xml_settings);


			//Преемственость - потом убрать
			rapidxml::xml_node<> *xml_bonuses = xml_settings->first_node("bonuses");
			if(xml_bonuses)
			{
				//std::string name = "bonus" + utils::lexical_cast(i);
				rapidxml::xml_attribute<> *xml_attr = xml_bonuses->first_attribute();
				size_t i = 0;
				while(xml_attr && i < 4)
				{
					levelSettings.setBool("Bonus" + utils::lexical_cast(i), utils::lexical_cast<bool>(xml_attr->value()));
					xml_attr = xml_attr->next_attribute();
					i++;
				}
			}
		}
		// посчитаем количество приемников, нужно знать в диалоге старта уровня
		totalReceivers = 0;
		for (rapidxml::xml_node<> *r = xml_level->first_node("EnergyReceivers")->first_node(); r; r = r->next_sibling()) {
			++totalReceivers;
		}
		// применяем значения баланса, полученные из Swrve
		std::map<std::string, std::string> abSettings = levelsInfo.GetLevelSettingsAB(EditorUtils::lastLoadedLevel);
		levelSettings.UpdateValues(abSettings);
	}
	
	void LoadLevel(rapidxml::xml_node<> *xml_level)
	{
		LoadLevelSettings(xml_level);

		for(size_t i = 0; i < editor_makers.size(); i++)
		{
			editor_makers[i]->LoadLevel(xml_level);
		}
		Gadgets::snapGadgets.CheckAfterLoadReceivers();
		Gadgets::energySpeedChangers.LoadLevel(xml_level);
		Gadgets::bombFields.LoadLevel(xml_level);

		std::string levelType = levelSettings.getString("LevelType");
		
		if( levelType == "moves" ) {
			Match3GUI::ActCounter::SetCounter(levelSettings.getInt("ActionsCount"));
			Match3GUI::TimeCounter::SetTime(std::numeric_limits<float>::max());
		} else if( levelType == "time" ) {
			Match3GUI::TimeCounter::SetTime((float)levelSettings.getInt("ActionsCount"));
			Match3GUI::ActCounter::SetCounter(std::numeric_limits<int>::max());
		}
		Match3GUI::ActCounter::ResetHiddenCounter();
		Match3GUI::TimeCounter::ResetHiddenTime();
	}

	void SaveWallTypes(Xml::TiXmlElement *root)
	{
		std::vector< std::pair<Game::FieldAddress, std::string> > info;

		for (int i = 0; i < GameSettings::fieldWidth; i++){
			for (int j = 0; j < GameSettings::fieldHeight; j++){
				Game::Square *sq = GameSettings::gamefield[i+1][j+1];
				if( sq->IsSand() )
					info.push_back( std::make_pair(Game::FieldAddress(i,j), "sand"));
				if( sq->IsIndestructible() )
					info.push_back( std::make_pair(Game::FieldAddress(i,j), "indestructible"));
				if( sq->IsPermanent() )
					info.push_back( std::make_pair(Game::FieldAddress(i,j), "permanent"));
				if( sq->IsRestoring() )
					info.push_back( std::make_pair(Game::FieldAddress(i,j), "restoring"));
				if( sq->GetWallColor() > 0 )
					info.push_back( std::make_pair(Game::FieldAddress(i,j), "color_" + utils::lexical_cast(sq->GetWallColor())));
			}
		}

		if(!info.empty())
		{
			Xml::TiXmlElement *xml_squares = root->InsertEndChild(Xml::TiXmlElement("WallTypesInfo"))->ToElement();

			for(std::vector< std::pair<Game::FieldAddress, std::string> >::iterator i = info.begin(); i != info.end(); i++)
			{
				Xml::TiXmlElement *xml_square_info = xml_squares->InsertEndChild(Xml::TiXmlElement("square"))->ToElement();
				i->first.SaveToXml(xml_square_info);
				xml_square_info->SetAttribute("type", i->second);
			}
		}
	}
	
	void Save(Xml::TiXmlElement *xml_level)
	{
		Xml::TiXmlElement *xml_before = xml_level->FirstChildElement();
		Xml::TiXmlElement *xml_settings = NULL;
		if(xml_before)
		{
			//Чтобы было в начале
			xml_settings = xml_level->InsertBeforeChild(xml_before, Xml::TiXmlElement("settings"))->ToElement();
		}else{
			xml_settings = xml_level->InsertEndChild(Xml::TiXmlElement("settings"))->ToElement();		
		}
		levelSettings.Save(xml_settings);
		//{ //попытка не сохранять дефолтовые значения. Работает, но до чего костыльно... ) (АндрейК)
		//	Xml::RapidXmlDocument doc_desc("GameDescriptions.xml");
		//	rapidxml::xml_node<> *xml_defalut = doc_desc.first_node()->first_node("level_setings_default");
		//	Xml::TiXmlElement *xml_elem = xml_settings->FirstChildElement("Data");
		//	while(xml_elem)
		//	{
		//		std::string name = Xml::GetStringAttribute(xml_elem, "name");
		//		rapidxml::xml_node<> *xml_default_elem = xml_defalut->first_node("Data");
		//		while(xml_default_elem)
		//		{
		//			if(name == Xml::GetStringAttribute(xml_default_elem, "name"))
		//			{
		//				break;
		//			}
		//			xml_default_elem = xml_default_elem->next_sibling("Data");						
		//		}
		//		if(xml_default_elem)
		//		{
		//			if(Xml::GetStringAttribute(xml_default_elem, "value") == Xml::GetStringAttribute(xml_elem, "value"))
		//			{
		//				Xml::TiXmlElement *xml_for_remove = xml_elem;
		//				xml_elem = xml_elem->NextSiblingElement("Data");
		//				xml_settings->RemoveChild(xml_for_remove);
		//				continue;
		//			}
		//		}
		//		xml_elem = xml_elem->NextSiblingElement("Data");
		//	}
		//}


		for(size_t i = 0; i < editor_makers.size(); i++)
		{
			editor_makers[i]->SaveLevel(xml_level);
		}

		Gadgets::energySpeedChangers.SaveLevel(xml_level);
		Gadgets::bombFields.SaveLevel(xml_level);
		Gadgets::ryushki.SaveLevel(xml_level);
		
		SaveWallTypes(xml_level);
	}
	// Функция удаляет какой-либо внешний объект c поля. То еcть такой, который не задаётcя
	// параметрами в Game::Square, а подcоединяетcя извне. Любая миниигра будет удалена этой функцией

	bool Editor_RemoveMinigameObject(Game::Square *cell, int button, bool totalRemove)
	{
		for(size_t i = 0; i < editor_makers.size(); i++)
		{
			if(editor_makers[i]->Editor_RightMouseDown(IPoint(-1, -1), cell))
			{
				//return true;
			}
		}
		return false;
	}
	void OnPushEditorButton()
	{	
		receivers.Editor_Reset();	
	}

	int LoadLevelDescription()
	{
		return gameInfo.getLocalInt("current_level", 0);
	}

	void DrawEdit()
	{
		for(size_t i = 0; i < editor_makers.size(); i++)
		{
			editor_makers[i]->DrawEdit();
		}
	}


	const int dx4[4] = { 0, 1,  0, -1 };
	const int dy4[4] = { 1, 0, -1,  0 };

	const int dx8[8] = { 0, 1, 0, -1, 1, -1, -1, 1};
	const int dy8[8] = { 1, 0, -1, 0, 1,  1, -1, -1};

	const int dx8ccw[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	const int dy8ccw[8] = {0, 1, 1, 1, 0, -1, -1, -1};


	void InitProcessSettings()
	{
		//Направления для растекания энергии
		bool diagonalEnergyAllow = Gadgets::levelSettings.getBool("DiagonalEnergy");
		if(diagonalEnergyAllow)
		{
			checkDirsInfo.count = 8;
			checkDirsInfo.dx = dx8;
			checkDirsInfo.dy = dy8;
		}else{
			checkDirsInfo.count = 4;
			checkDirsInfo.dx = dx4;
			checkDirsInfo.dy = dy4;
		}

		checkDirsChains.count = 8;
		checkDirsChains.dx = dx8ccw;
		checkDirsChains.dy = dy8ccw;
	}


}//namespace Gadgets

