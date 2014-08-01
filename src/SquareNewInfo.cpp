#include "stdafx.h"
#include "SquareNewInfo.h"
#include "Game.h"
#include "EditorUtils.h"
#include "Energy.h"

namespace Gadgets
{
	InfoSquare::InfoSquare()
		: portal("")
		, is_short_square(false)
		, forbid_bonus(false)
		, wallRemovedWithSeqFromEnergy(false)
		, growingWall(false)
		, growingWall_fabric(false)
		, isNoChainOnStart(false)
		, kill_diamonds(false)
		, treasure(0)
		, en_source(0)
	{}

	bool InfoSquare::empty() const
	{
		if(!portal.empty())
			return false;

		if(is_short_square)
			return false;

		if(forbid_bonus)
			return false;

		if(wallRemovedWithSeqFromEnergy)
			return false;

		if(growingWall)
			return false;

		if(isNoChainOnStart)
			return false;

		if(treasure > 0)
			return false;

		if(en_source > 0)
			return false;

		if(kill_diamonds)
			return false;

		return true;
	}

	int NEXT_EN_SOURCE_ID = 1;
	Render::Texture *texture_NoChain = NULL;
	void SquareNewInfo::Init(GameField *field, const bool &with_editor)
	{
		BaseEditorMaker::Init(field, with_editor);
		Clear();
		_type = "portal";
		//_stateEditPortal = PORTAL_FROM;
		addres_from = Game::FieldAddress();

		_PORTAL_IDS.clear();
		_PORTAL_IDS.push_back("#FF0000");
		_PORTAL_IDS.push_back("#00FF00");
		_PORTAL_IDS.push_back("#0000FF");

		_PORTAL_IDS.push_back("#FFFF00");
		_PORTAL_IDS.push_back("#FF00FF");
		_PORTAL_IDS.push_back("#FF8C00");
		_PORTAL_IDS.push_back("#6633FF");
		_PORTAL_IDS.push_back("#D2691E");
		_PORTAL_IDS.push_back("#00FFFF");
		_current_portal = 0;

		_prev_square = NULL;

		if(with_editor){
			texture_NoChain = Core::resourceManager.Get<Render::Texture>("NoChain");
		}else{
			texture_NoChain = NULL;
		}
	}

	void SquareNewInfo::LoadLevel(rapidxml::xml_node<> *root)
	{
		Clear();
		rapidxml::xml_node<> *xml_squares = root->first_node("NewSquareInfo");
		if(!xml_squares)
		{
			return;
		}
		rapidxml::xml_node<> *xml_square_info = xml_squares->first_node("square");
		while(xml_square_info)
		{
			Game::FieldAddress fa(xml_square_info);
			InfoSquare &info = _info[fa];
			if(info.empty())
			{
				info.portal = Xml::GetStringAttributeOrDef(xml_square_info, "portal", "");
				if(info.portal == "RED"){
					info.portal = "#FF0000";
				}else if(info.portal == "GREEN"){
					info.portal =  "#00FF00";
				}else if(info.portal == "BLUE"){
					info.portal =  "#0000FF";
				}
			}
			if(info.empty())
			{
				info.is_short_square = Xml::GetBoolAttributeOrDef(xml_square_info, "is_short_square", false);
			}
			if(info.empty())
			{
				info.forbid_bonus = Xml::GetBoolAttributeOrDef(xml_square_info, "no_bonus", false);
			}
			info.wallRemovedWithSeqFromEnergy = Xml::GetBoolAttributeOrDef(xml_square_info, "energy_wall", false);

			info.growingWall = Xml::GetBoolAttributeOrDef(xml_square_info, "growing_wall", false);
			if(info.growingWall) {
				info.growingWall_fabric = Xml::GetBoolAttributeOrDef(xml_square_info, "fabric", false);
			}
			info.isNoChainOnStart = Xml::GetBoolAttributeOrDef(xml_square_info, "no_chain", false);
			info.treasure = Xml::GetIntAttributeOrDef(xml_square_info, "treasure", 0);
			info.en_source = Xml::GetIntAttributeOrDef(xml_square_info, "en_source", 0);
			info.kill_diamonds = Xml::GetBoolAttributeOrDef(xml_square_info, "kill_diamonds", false);

			xml_square_info = xml_square_info->next_sibling("square");
		}
		Apply();
	}

	void SquareNewInfo::SaveLevel(Xml::TiXmlElement *root)
	{
		if(_info.empty())
		{
			return;
		}
		Xml::TiXmlElement *xml_squares = root -> InsertEndChild(Xml::TiXmlElement("NewSquareInfo")) -> ToElement();
		Xml::TiXmlElement *xml_square_info;

		std::map<Game::FieldAddress, InfoSquare>::iterator i = _info.begin();
		InfoSquare simpleInfo;
		for(; i!= _info.end(); i++)
		{
			if(i->second.empty())
			{
				continue;
			}
			
			xml_square_info = xml_squares -> InsertEndChild(Xml::TiXmlElement("square")) -> ToElement();
			
			//пишем в алфавитном порядке !!!
			if(i->second.en_source > 0)
			{
				xml_square_info->SetAttribute("en_source", utils::lexical_cast(i->second.en_source));					
			}
			if(i->second.wallRemovedWithSeqFromEnergy)
			{
				xml_square_info->SetAttribute("energy_wall", utils::lexical_cast(true));	
			}

			if(i->second.growingWall)
			{
				xml_square_info->SetAttribute("fabric", utils::lexical_cast(i->second.growingWall_fabric));
				xml_square_info->SetAttribute("growing_wall", utils::lexical_cast(true));
			}

			//i->first.SaveToXml(xml_square_info);
			
			xml_square_info->SetAttribute("i", i->first.GetCol());
			if(i->second.is_short_square)
			{
				xml_square_info->SetAttribute("is_short_square", utils::lexical_cast(true));	
			}
			xml_square_info->SetAttribute("j", i->first.GetRow());
			if(i->second.kill_diamonds)
			{
				xml_square_info->SetAttribute("kill_diamonds", utils::lexical_cast(true));
			}
			if(i->second.forbid_bonus)
			{
				xml_square_info->SetAttribute("no_bonus", utils::lexical_cast(true));
			}
			if(i->second.isNoChainOnStart)
			{
				xml_square_info->SetAttribute("no_chain", utils::lexical_cast(true));
			}
			if(!i->second.portal.empty())
			{
				xml_square_info->SetAttribute("portal", i->second.portal.c_str());
			}
			if(i->second.treasure > 0)
			{
				xml_square_info->SetAttribute("treasure", i->second.treasure);
			}
		}	
	}

	void SquareNewInfo::Apply(const Game::FieldAddress &address)
	{
		EnergySource_UpgradeIndexes();
		if(_info.find(address) == _info.end())
		{
			if(address.IsValid())
			{
				GameSettings::gamefield[address]->SetInfo(InfoSquare());
			}
		}else{
			if(_info.find(address)->second.empty())
			{
				_info.erase(_info.find(address));
				GameSettings::gamefield[address]->SetInfo(InfoSquare());
			}else{
				GameSettings::gamefield[address]->SetInfo(_info.find(address)->second);			
			}
		}
	}

	void SquareNewInfo::Apply()
	{
		EnergySource_UpgradeIndexes();
		for(std::map<Game::FieldAddress, InfoSquare>::iterator it = _info.begin(); it != _info.end(); )
		{
			Game::Square *sq = GameSettings::gamefield[it->first];
			if(Game::isSquare(sq))
			{
				sq->SetInfo(it->second);
				if(it->second.empty()) {
					_info.erase(it++);
				} else {
					++it;
				}
			} else {
				++it;
			}
		}
	}

	void SquareNewInfo::Clear()
	{
		_prev_square = NULL;
		_info.clear();
	}

	void SquareNewInfo::Release()
	{
		Clear();
	}

	bool SquareNewInfo::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!Game::isSquare(sq))
		{
			if(EditorUtils::activeEditBtn == EditorUtils::ShortSquareInfo) {
				// в качестве исключения позволим тыкать этим инструментом на пустые клетки (создаем новую клетку в таком случае)
				Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
				sq = Game::GetValidSquare(index);
			} else {
				return false;
			}
		}
		_prev_square = sq;

		Gadgets::InfoSquare &info = _info[sq->address];

		if(EditorUtils::activeEditBtn == EditorUtils::SquareNewInfo) {
			info.portal = _PORTAL_IDS[_current_portal];
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::ShortSquareInfo) {
			info.is_short_square = true;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::ForbidHangBonus) {
			info.forbid_bonus = true;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::WallWithEnergySeq) {
			info.wallRemovedWithSeqFromEnergy = true;
			info.is_short_square = false;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::WallGrowing) {
			info.growingWall = true;
			info.growingWall_fabric = Core::mainInput.IsControlKeyDown();
			info.is_short_square = false;
			Apply(sq->address);
		} else if(EditorUtils::activeEditBtn == EditorUtils::NoChainOnStart) {
			info.isNoChainOnStart = true;
			Apply(sq->address);
		} else if(EditorUtils::activeEditBtn == EditorUtils::TreasureGround) {
			if( info.treasure == 0 )
				info.treasure = 1;
			else
				info.treasure = info.treasure % 3 + 1;
			Apply(sq->address);
		} else if(EditorUtils::activeEditBtn == EditorUtils::EnergySource) {
			info.en_source = Gadgets::NEXT_EN_SOURCE_ID;
			info.is_short_square = false;
			info.wallRemovedWithSeqFromEnergy = false;
			info.growingWall = false;
			info.growingWall_fabric = false;
			GameSettings::gamefield[sq->address]->SetWall(0);
			Apply(sq->address);
		} else if(EditorUtils::activeEditBtn == EditorUtils::KillDiamonds) {
			info.kill_diamonds = !info.kill_diamonds;
			Apply(sq->address);
		}
		return false;
	}
		
	bool SquareNewInfo::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!Game::isSquare(sq))
		{
			return false;
		}
		_prev_square = sq;
		std::map<Game::FieldAddress, InfoSquare>::iterator itr = _info.find(sq->address);

		if(itr == _info.end())
			return false;

		InfoSquare &info = itr->second;

		if( EditorUtils::activeEditBtn == EditorUtils::None || EditorUtils::activeEditBtn == EditorUtils::Null) {
			info = InfoSquare();
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::SquareNewInfo) {
			info.portal.clear();
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::ShortSquareInfo) {
			info.is_short_square = false;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::ForbidHangBonus) {
			info.forbid_bonus = false;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::WallWithEnergySeq) {
			info.wallRemovedWithSeqFromEnergy = false;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::WallGrowing) {
			info.growingWall = false;
			info.growingWall_fabric = false;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::NoChainOnStart) {
			info.isNoChainOnStart = false;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::TreasureGround) {
			info.treasure = 0;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::EnergySource) {
			info.en_source = 0;
			Apply(sq->address);
			return true;
		} else if(EditorUtils::activeEditBtn == EditorUtils::KillDiamonds) {
			info.kill_diamonds = false;
			Apply(sq->address);
			return true;
		}

		return false;
	}

	bool SquareNewInfo::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(_prev_square != sq)
		{
			if( Core::mainInput.GetMouseLeftButton())
				return Editor_LeftMouseDown(mouse_pos, sq);
			if( Core::mainInput.GetMouseRightButton())
				return Editor_RightMouseDown(mouse_pos, sq);
		}
		return false;
	}

	bool SquareNewInfo::Editor_MouseWheel(int delta, Game::Square *sq)
	{
		if(EditorUtils::activeEditBtn == EditorUtils::SquareNewInfo)
		{
			_current_portal = (_current_portal + delta + _PORTAL_IDS.size()) % _PORTAL_IDS.size();
			return true;
		}
		return false;
	}

	void SquareNewInfo::DrawEdit()
	{
		if(EditorUtils::activeEditBtn == EditorUtils::SquareNewInfo)
		{
			IRect rect = IRect(10, 100, 50 , 20);
			Render::device.SetTexturing(false);
			Render::BeginColor(Color(50, 50, 50, 255));
			Render::DrawRect(rect);
			Render::EndColor();
			Render::device.SetTexturing(true);

			Render::FreeType::BindFont("debug");
			std::string string_info = _type + " current:" + _PORTAL_IDS[_current_portal];
			Render::PrintString( rect.x + rect.width/2, rect.y + rect.height/2, string_info, 1.f, CenterAlign, CenterAlign); 
		}
		if(texture_NoChain)
		{
			//Отрисовка мест без цепочек
			for(std::map<Game::FieldAddress, InfoSquare>::iterator i = _info.begin(); i != _info.end(); i++)
			{
				if(i->second.isNoChainOnStart)
				{
					FPoint pos = i->first.ToPoint()*GameSettings::SQUARE_SIDE;
					texture_NoChain->Draw(pos);
				}
			}
		}
		for(std::map<Game::FieldAddress, InfoSquare>::iterator i = _info.begin(); i != _info.end();i++)
		{
			// помечаем клетку-иcточник энергии
			if(i->second.en_source > 0)
			{
				Render::BindFont("debug");
				FPoint pos = GameSettings::CellCenter(i->first.ToPoint());
				Render::PrintString(pos, "e(" + utils::lexical_cast(i->second.en_source) + ")", 1.f, CenterAlign, BaseLineAlign);
			}
			if(i->second.treasure > 0)
			{
				Render::BindFont("Score");
				FPoint pos = GameSettings::CellCenter(i->first.ToPoint());
				Render::PrintString(pos, utils::lexical_cast(i->second.treasure), 1.f, CenterAlign, CenterAlign);
			}
			if(i->second.kill_diamonds)
			{
				Render::BindFont("debug");
				FPoint pos = GameSettings::CellCenter(i->first.ToPoint()) - FPoint(0.0f, GameSettings::SQUARE_SIDEF * 0.25f);
				Render::BeginColor(Color::RED);
				Render::PrintString(pos, "D", 1.2f, CenterAlign, CenterAlign);
				Render::EndColor();
			}
		}
	}

	bool SquareNewInfo::AcceptMessage(const Message &message)
	{
		if(EditorUtils::activeEditBtn != EditorUtils::SquareNewInfo)
		{
			return false;
		}
		if(message.is("KeyPress"))
		{
			int key = utils::lexical_cast<int> (message.getData());
			if(key == ' ')
			{
				if(_type == "portal")
				{
					_type = "portal";
				}
				return true;
			}
		}
		return false;
	}

	void SquareNewInfo::Editor_ClearFieldPart(IRect part)
	{
		for(std::map<Game::FieldAddress, InfoSquare>::iterator itr = _info.begin(); itr != _info.end(); )
		{
			if( part.Contains(itr->first.ToPoint()) ) {
				itr = _info.erase(itr);
			} else {
				++itr;
			}
		}
	}

	void SquareNewInfo::Editor_CopyToClipboard(IRect part)
	{
		Game::FieldAddress offset(part.x, part.y);
		_clipboard.clear();
		for(std::map<Game::FieldAddress, InfoSquare>::iterator itr = _info.begin(); itr != _info.end(); ++itr)
		{
			if( part.Contains(itr->first.ToPoint()) )
				_clipboard.insert( std::make_pair(itr->first - offset, itr->second) );
		}
	}

	void SquareNewInfo::Editor_CutToClipboard(IRect part)
	{
		Editor_CopyToClipboard(part);
		Editor_ClearFieldPart(part);
	}

	bool SquareNewInfo::Editor_PasteFromClipboard(IPoint pos)
	{
		Game::FieldAddress offset(pos);
		for(std::map<Game::FieldAddress, InfoSquare>::iterator itr = _clipboard.begin(); itr != _clipboard.end(); ++itr)
		{
			_info[itr->first + offset] = itr->second;
		}
		_clipboard.clear();
		Apply();
		return true;
	}

	bool SquareNewInfo::IsEnergySourceSquare(const IPoint &address) const
	{
		std::map<Game::FieldAddress, InfoSquare>::const_iterator itr = _info.find(Game::FieldAddress(address));
		return (itr != _info.end()) && (itr->second.en_source > 0);
	}

	void SquareNewInfo::EnergySource_Start()
	{
		for(std::map<Game::FieldAddress, InfoSquare>::iterator i = _info.begin(); i != _info.end();i++)
		{
			if(i->second.en_source <= 0)
			{
				continue;
			}
			Game::Square *sq = GameSettings::gamefield[ i->first];
			MyAssert(sq && sq != Game::bufSquare);
			Energy::field.StartEnergyInSquare(i->first);
			sq->SetEnergyChecked();
		}
	}

	void SquareNewInfo::EnergySource_Get(std::vector<IPoint> &vec)
	{
		std::multimap<int, IPoint> sorted_map;
		for(std::map<Game::FieldAddress, InfoSquare>::iterator i = _info.begin(); i != _info.end();i++)
		{
			if(i->second.en_source > 0)
			{
				sorted_map.insert(std::pair<int, IPoint>(i->second.en_source, i->first.ToPoint()));
			}	
		}
		vec.clear();
		for(std::multimap<int, IPoint>::iterator i = sorted_map.begin(); i != sorted_map.end();i++)
		{
			vec.push_back(i->second);
		}
	}

	void SquareNewInfo::EnergySource_UpgradeIndexes()
	{
		std::vector<IPoint> vec;
		//Пересчитаем индексы всех источников энергии чтобы шли по порядку
		EnergySource_Get(vec);
		for(size_t i = 0; i < vec.size(); i++)
		{
			_info[Game::FieldAddress(vec[i])].en_source = i+1;		
		}	
		Gadgets::NEXT_EN_SOURCE_ID = vec.size() + 1;
	}

	SquareNewInfo square_new_info;
}//Gadgets