#include "stdafx.h"
#include "GroundInfo.h"
#include "Game.h"
#include "EditorUtils.h"

namespace Gadgets
{

	void GroundMaker::Init(GameField *field, const bool &with_editor)
	{
		BaseEditorMaker::Init(field, with_editor);
		Clear();
	}

	void GroundMaker::LoadLevel(rapidxml::xml_node<> *root)
	{
		Clear();
		rapidxml::xml_node<> *xml_squares = root->first_node("GroundInfo");
		if(!xml_squares)
		{
			return;
		}
		rapidxml::xml_node<> *xml_square_info = xml_squares->first_node("square");
		while(xml_square_info)
		{
			Game::FieldAddress a = Game::FieldAddress(xml_square_info);
			_info.insert(std::pair<Game::FieldAddress, WoodInfo>(a, WoodInfo()));
			WoodInfo &info = _info[a];
			info.wood = Xml::GetIntAttributeOrDef(xml_square_info, "ground", 0);
			info.isGrowingWood = Xml::GetBoolAttributeOrDef(xml_square_info, "is_growing", false);
			info.isGoldWood = Xml::GetBoolAttributeOrDef(xml_square_info, "is_gold", false);
			xml_square_info = xml_square_info->next_sibling("square");
		}
		Apply();
	}

	void GroundMaker::SaveLevel(Xml::TiXmlElement *root)
	{
		if(_info.empty())
		{
			return;
		}
		Xml::TiXmlElement *xml_squares = root -> InsertEndChild(Xml::TiXmlElement("GroundInfo")) -> ToElement();
		Xml::TiXmlElement *xml_square_info;

		std::map<Game::FieldAddress, WoodInfo>::iterator i = _info.begin();
		WoodInfo simpleInfo;
		for(; i!= _info.end(); i++)
		{
			xml_square_info = xml_squares -> InsertEndChild(Xml::TiXmlElement("square")) -> ToElement();
			
			//по алфавиту!
			//i->first.SaveToXml(xml_square_info);

			xml_square_info->SetAttribute("ground", i->second.wood);
			xml_square_info->SetAttribute("i", i->first.GetCol());
			if(i->second.isGrowingWood)
			{
				xml_square_info->SetAttribute("is_growing", i->second.isGrowingWood);
			}
			if(i->second.isGoldWood)
			{
				xml_square_info->SetAttribute("is_gold", i->second.isGoldWood);
			}
			xml_square_info->SetAttribute("j", i->first.GetRow());
		}	
	}

	void GroundMaker::Apply()
	{
		//Очищаем придыдущие значения
		for (int i = 0; i < GameSettings::fieldWidth; i++)
		{
			for (int j = 0; j < GameSettings::fieldHeight; j++)
			{
				Game::Square *sq = GameSettings::gamefield[i+1][j+1];

				if ( !Game::isBuffer(sq) )
				{
					sq->SetWood(0);
					sq->SetEnergyWood(false);
					sq->SetGoldWood(false);
				}
			}
		}
		if(_info.empty())
		{
			return;
		}
		std::map<Game::FieldAddress, WoodInfo>::iterator i = _info.begin();
		std::list<Game::FieldAddress> eraseList;
		for(; i!= _info.end();)
		{
			if(i->second.wood == 0)
			{
				eraseList.push_back(i->first);
				i++;
				continue;	
			}
			Game::Square *sq = Game::GetValidSquare(i->first);
			if(i->second.wood >= 0) {
				sq->SetWood(i->second.wood);
				sq->SetEnergyWood(false);
				sq->SetGrowingWood(i->second.isGrowingWood); //там переопределится что высота всегда 3
				sq->SetGoldWood(i->second.isGoldWood);
			} else {
				sq->SetWood(0);
				sq->SetEnergyWood(true);
				sq->SetGrowingWood(false);
			}
			i++;
		}
		for (std::list<Game::FieldAddress>::iterator i = eraseList.begin(), e = eraseList.end(); i != e; ++i) {
			_info.erase(*i);
		}
	}

	void GroundMaker::Clear()
	{
		_prev_square = NULL;
		_info.clear();
	}

	void GroundMaker::Release()
	{
		Clear();
	}

	bool GroundMaker::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
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

	bool GroundMaker::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
	{
		return false;
	}

	bool GroundMaker::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!Game::isSquare(sq))
		{
			return false;
		}
		_prev_square = sq;
		Gadgets::WoodInfo &info = _info[sq->address];
		if(EditorUtils::activeEditBtn == EditorUtils::AddWood)
		{
			info.wood = std::min(5, info.wood + 1);
			info.isGrowingWood = false;
			info.isGoldWood = Core::mainInput.IsControlKeyDown();
			Apply();			
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::EnergyGround)
		{
			info.wood = -1;
			info.isGrowingWood = false;
			info.isGoldWood = false;
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::GrowingWoodEdit)
		{
			info.wood = 3;
			info.isGrowingWood = true;
			info.isGoldWood = false;
			Apply();
			return true;
		}
		return false;
	}
		
	bool GroundMaker::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!sq)
		{
			return false;
		}

		if( EditorUtils::activeEditBtn == EditorUtils::AddWood
			|| EditorUtils::activeEditBtn == EditorUtils::EnergyGround
			|| EditorUtils::activeEditBtn == EditorUtils::GrowingWoodEdit
			|| EditorUtils::activeEditBtn == EditorUtils::None
			|| EditorUtils::activeEditBtn == EditorUtils::Null )
		{
			_prev_square = sq;
			std::map<Game::FieldAddress, WoodInfo>::iterator it = _info.find(sq->address);
			if(it != _info.end())
			{
				if(EditorUtils::activeEditBtn == EditorUtils::AddWood && !it->second.isGrowingWood && it->second.wood > 1) {
					it->second.wood--;
				} else {
					_info.erase(it);
				}
				Apply();
				return true;
			}
		}
		return false;
	}

	void GroundMaker::Editor_ClearFieldPart(IRect part)
	{
		for(std::map<Game::FieldAddress, WoodInfo>::iterator itr = _info.begin(); itr != _info.end(); )
		{
			if( part.Contains(itr->first.ToPoint()) ) {
				itr = _info.erase(itr);
			} else {
				++itr;
			}
		}
	}

	void GroundMaker::Editor_CopyToClipboard(IRect part)
	{
		Game::FieldAddress offset(part.x, part.y);
		_clipboard.clear();
		for(std::map<Game::FieldAddress, WoodInfo>::iterator itr = _info.begin(); itr != _info.end(); ++itr)
		{
			if( part.Contains(itr->first.ToPoint()) )
				_clipboard.insert( std::make_pair(itr->first - offset, itr->second) );
		}
	}

	void GroundMaker::Editor_CutToClipboard(IRect part)
	{
		Editor_CopyToClipboard(part);
		Editor_ClearFieldPart(part);
	}

	bool GroundMaker::Editor_PasteFromClipboard(IPoint pos)
	{
		Game::FieldAddress offset(pos);
		for(std::map<Game::FieldAddress, WoodInfo>::iterator itr = _clipboard.begin(); itr != _clipboard.end(); ++itr)
		{
			_info[itr->first + offset] = itr->second;
		}
		_clipboard.clear();
		Apply();
		return true;
	}

}//Gadgets