#include "stdafx.h"
#include "ChipsInfo.h"
#include "EditorUtils.h"
#include "GameBonuses.h"
#include "Game.h"

namespace Gadgets
{
	Render::Texture *chipsTex;

	void ChipsInfo::Init(GameField *field, const bool &with_editor)
	{
		BaseEditorMaker::Init(field, with_editor);
		_chip_arrows_length = 1;
		_current_pre_chip = 1;
		_prev_square = NULL;
		Clear();
	}

	void ChipsInfo::LoadLevel(rapidxml::xml_node<> *root)
	{
		Game::diamondsFound = 0;

		Clear();
		rapidxml::xml_node<> *xml_info = root->first_node("ChipWithActions");
		if(!xml_info)
		{
			return;
		}
		rapidxml::xml_node<> *xml_chip_info = xml_info->first_node("chip");
		while(xml_chip_info)
		{
			Game::FieldAddress address = Game::FieldAddress(xml_chip_info);
			InfoC &info = _info[address];
			info.act_count = Xml::GetIntAttributeOrDef(xml_chip_info, "count_act", 0);
			info.time_bomb = Xml::GetIntAttributeOrDef(xml_chip_info, "time_bomb", 0);
			info.star = Xml::GetIntAttributeOrDef(xml_chip_info, "star", 0);
			info.lock = Xml::GetIntAttributeOrDef(xml_chip_info, "lock", 0);
			info.adapt = Xml::GetBoolAttributeOrDef(xml_chip_info, "adapt", false);
			
			info.pre_chip = Xml::GetIntAttributeOrDef(xml_chip_info, "pre_chip", -1);
			info.pre_chameleon = Xml::GetBoolAttributeOrDef(xml_chip_info, "pre_chameleon", false);
			info.treasure = Xml::GetIntAttributeOrDef(xml_chip_info, "treasure", 0);

			if( Xml::HasAttribute(xml_chip_info, "type") ) {
				info.type = Game::ChipColor::Type( Xml::GetIntAttributeOrDef(xml_chip_info, "type", Game::ChipColor::CHIP) );
			} else {
				// для совместимости со старым форматом уровней
				if( Xml::GetBoolAttributeOrDef(xml_chip_info, "key", false) )
					info.type = Game::ChipColor::KEY;
				else if( Xml::GetBoolAttributeOrDef(xml_chip_info, "licorice", false) )
					info.type = Game::ChipColor::LICORICE;
				else if( Xml::GetBoolAttributeOrDef(xml_chip_info, "moving_monster", false) )
					info.type = Game::ChipColor::THIEF;
				else if( info.star > 0 )
					info.type = Game::ChipColor::STAR;
			}
			if(info.type == Game::ChipColor::STAR && info.star == 0)
			{
				info.type = Game::ChipColor::CHIP;
			}

			info.level_hang.Load(xml_chip_info);
			xml_chip_info = xml_chip_info->next_sibling("chip");
		}
		if(true) //Преемственность
		{
			//Предустановленные фишки
			rapidxml::xml_node<> *xml_chips = root->first_node("PreInstalledChips");
			if(xml_chips){
				rapidxml::xml_node<> *xml_chip  = xml_chips->first_node("Chip");
				while(xml_chip){
					IPoint pos(xml_chip);
					Game::FieldAddress address = Game::FieldAddress(pos.x, pos.y);
					_info[address].pre_chip = Xml::GetIntAttributeOrDef(xml_chip, "color", -1);
					xml_chip = xml_chip->next_sibling("Chip");
				}
			}
			//Хамелеоны
			for(size_t i = 0; i < GameSettings::squares.size(); i++)
			{
				if(GameSettings::squares[i]->GetChip().IsChameleon())
				{
					_info[GameSettings::squares[i]->address].pre_chameleon = true;
				}
			}
		}
		Apply();
	}

	void ChipsInfo::SaveLevel(Xml::TiXmlElement *root)
	{
		if(_info.empty())
		{
			return;
		}
		Xml::TiXmlElement *xml_chips = root -> InsertEndChild(Xml::TiXmlElement("ChipWithActions")) -> ToElement();
		Xml::TiXmlElement *xml_chip;

		std::map<Game::FieldAddress, InfoC>::iterator i = _info.begin();
		for(; i!= _info.end(); i++)
		{
			if(i->second.IsEmpty())
			{
				continue;
			}
			//по алфавиту!
			xml_chip = xml_chips -> InsertEndChild(Xml::TiXmlElement("chip")) -> ToElement();
			if(i->second.adapt)
			{
				xml_chip->SetAttribute("adapt", utils::lexical_cast(i->second.adapt).c_str());
			}
			if(i->second.act_count > 0)
			{
				xml_chip->SetAttribute("count_act", i->second.act_count);
			}
			xml_chip->SetAttribute("i", i->first.GetCol());
			xml_chip->SetAttribute("j", i->first.GetRow());
			if(i->second.lock > 0)
			{
				xml_chip->SetAttribute("lock", utils::lexical_cast(i->second.lock).c_str());
			}
			if(i->second.star)
			{
				xml_chip->SetAttribute("star", utils::lexical_cast(i->second.star).c_str());
			}
			if(i->second.pre_chameleon)
			{
				xml_chip->SetAttribute("pre_chameleon", utils::lexical_cast(i->second.pre_chameleon).c_str());
			}
			if(i->second.pre_chip != -1)
			{
				xml_chip->SetAttribute("pre_chip", i->second.pre_chip);
			}
			if(i->second.time_bomb > 0)
			{
				xml_chip->SetAttribute("time_bomb", i->second.time_bomb);
			}
			if(i->second.treasure > 0)
			{
				xml_chip->SetAttribute("treasure", i->second.treasure);
			}
			if(i->second.type != Game::ChipColor::CHIP)
			{
				xml_chip->SetAttribute("type", i->second.type);
			}
			if(!i->second.level_hang.IsEmpty())
			{
				i->second.level_hang.Save(xml_chip);
			}
		}	
	}

	void ChipsInfo::Apply()
	{
		for(std::map<Game::FieldAddress, InfoC>::iterator itr = _info.begin(); itr != _info.end(); )
		{
			Game::Square *sq = GameSettings::gamefield[itr->first];
			if( Game::isVisible(sq) && (sq->barrierIndex == -1) )
			{
				sq->GetChip().SetInfo(itr->second, itr->first.ToPoint());
				if(itr->second.star > 0 || itr->second.treasure > 0)
					{
						sq->SetWall(0);
					}
			}
			if(itr->second.IsEmpty()) {
				_info.erase(itr++);
			} else {
				++itr;
			}
		}
	}

	void ChipsInfo::Clear()
	{
		_info.clear();
	}

	void ChipsInfo::Release()
	{
		Clear();
	}

	bool ChipsInfo::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!Game::isSquare(sq)){	
			return false;
		}
		if(sq->barrierIndex != -1)
		{
			return false;
		}
		_prev_square = sq;
		InfoC &info = _info[sq->address];
		if( EditorUtils::activeEditBtn == EditorUtils::ChameleonParams)
		{
			if(!info.pre_chameleon)
			{
				info.pre_chameleon = true;
				info.pre_chip = -1;
				info.star = 0;
				info.type = Game::ChipColor::CHIP;
				info.adapt = false;
			}
			Apply();		
		}
		else if( EditorUtils::activeEditBtn == EditorUtils::PreInstalledChip)
		{
			_info[sq->address].pre_chip = _current_pre_chip;
			if(info.pre_chip >= 0)
			{
				info.star = 0;
				info.adapt = false;
				info.type = Game::ChipColor::CHIP;
				info.pre_chameleon = false;
			}
			Apply();
			return true;			
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::PregenerateChip)
		{
			info.pre_chip = -2;
			info.star = 0;
			info.adapt = false;
			info.type = Game::ChipColor::CHIP;
			info.pre_chameleon = false;
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipStar)
		{
			info.act_count = 0;
			info.treasure = 0;
			info.type = Game::ChipColor::STAR;
			if(info.star == 0)
			{
				info.adapt = false;
				info.pre_chip = -1;
				info.pre_chameleon = false;
			}
			info.star = info.star + 1;
			if(info.star > 3)
			{
				info.star = 1;
			}
			info.level_hang.Clear();
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::TreasureChip)
		{
			info.act_count = 0;
			info.star = 0;
			info.type = Game::ChipColor::TREASURE;
			if( info.treasure == 0)
			{
				info.adapt = false;
				info.pre_chip = -1;
				info.pre_chameleon = false;
			}
			info.treasure = info.treasure + 1;
			if( info.treasure > 3) {
				info.treasure = 1;
			}
			info.level_hang.Clear();
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipLockEditor)
		{
			info.lock = (info.lock + 1) % 3;
			info.level_hang.Clear();
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipKey)
		{
			if(info.type != Game::ChipColor::KEY)
			{
				info.type = Game::ChipColor::KEY;
				info.star = 0;
				info.adapt = false;
				info.pre_chip = -1;
				info.pre_chameleon = false;
				info.level_hang.Clear();
			}
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipLicorice)
		{
			if(!info.type == Game::ChipColor::LICORICE && !sq->IsIce())
			{
				info.type = Game::ChipColor::LICORICE;
				info.star = 0;
				info.adapt = false;
				info.pre_chip = -1;
				info.pre_chameleon = false;
				info.level_hang.Clear();
			}
			Apply();
			return true;	
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::MovingMonsterEdit && !Core::mainInput.IsControlKeyDown())
		{  //с контролом ставит источник монстриков
			if(info.type != Game::ChipColor::THIEF  && !sq->IsIce())
			{
				info.type = Game::ChipColor::THIEF;
				info.star = 0;
				info.adapt = false;
				info.pre_chip = -1;
				info.pre_chameleon = false;
				info.level_hang.Clear();
			}
			Apply();
			return true;	
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipAdapt)
		{
			info.type = Game::ChipColor::CHIP;
			if(!info.adapt)
			{
				info.adapt = true;
				info.star = 0;
				info.pre_chip = -1;
				info.pre_chameleon = false;
			}
			Apply();
			return true;	
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipActions)
		{
			info.type = Game::ChipColor::CHIP;
			info.act_count++;
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipTimeBomb)
		{
			info.type = Game::ChipColor::CHIP;
			info.time_bomb++;
			info.level_hang.Clear();
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::DiamondChip && !Core::mainInput.IsControlKeyDown())
		{
			info.type = Game::ChipColor::DIAMOND;
			info.level_hang.Clear();
			info.act_count = 0;
			info.time_bomb = 0;
			info.lock = 0;
			info.star = 0;
			Apply();
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipArrows)
		{
			//Обнуляем
			info.act_count = 0;
			info.time_bomb = 0;
			info.lock = 0;
			info.star = 0;
			info.type = Game::ChipColor::CHIP;

			//Hang
			Game::ArrowBonus *arrowHang = (Game::ArrowBonus*)_info[sq->address].level_hang.GetBonusByType(Game::HangBonus::ARROW);
			if( !arrowHang ) {
				_info[sq->address].level_hang.MakeArrow(_chip_arrows_length, 0);
				arrowHang = (Game::ArrowBonus*)_info[sq->address].level_hang.GetBonusByType(Game::HangBonus::ARROW);
			}
			arrowHang->SetDirections( arrowHang->GetDirections() ^ (1 << _chip_arrows_dir) );
			if( arrowHang->GetDirections() > 0){
				arrowHang->SetRadius(_chip_arrows_length);
			} else {
				_info[sq->address].level_hang.Clear();
			}		
			Apply();
		}

		//фишка с бомбой
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipWithBomb)
		{
			//Обнуляем
			info.act_count = 0;
			info.time_bomb = 0;
			info.lock = 0;
			info.star = 0;
			info.type = Game::ChipColor::CHIP;

			//Hang
			Game::BombBonus *bombHang = (Game::BombBonus*)_info[sq->address].level_hang.GetBonusByType(Game::HangBonus::BOMB);
			if( !bombHang ) {
				_info[sq->address].level_hang.MakeBomb(1, 1);
			}
			Apply();
		}
		return false;
	}
		
	bool ChipsInfo::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!sq)
		{
			return false;
		}
		_prev_square = sq;

		auto itr = _info.find(sq->address);
		if( itr == _info.end() )
			return false;

		bool changed = false;
		InfoC &info = itr->second;

		if( EditorUtils::activeEditBtn == EditorUtils::None || EditorUtils::activeEditBtn == EditorUtils::Null )
		{
			info = InfoC();
			changed = true;
		}
		else if( EditorUtils::activeEditBtn == EditorUtils::ChameleonParams)
		{
			if(info.pre_chameleon) {
				info.pre_chameleon = false;
				changed = true;
			}	
		}
		else if( EditorUtils::activeEditBtn == EditorUtils::PreInstalledChip)
		{
			if(info.pre_chip >= 0){
				info.pre_chip = -1;
				changed = true;
			}			
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::PregenerateChip)
		{
			if(info.pre_chip == -2) {
				info.pre_chip = -1;
				changed = true;
			}
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipStar)
		{
			changed = (info.star > 0);
			info.star = 0;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::TreasureChip)
		{
			changed = (info.treasure > 0);
			info.treasure = 0;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipLockEditor)
		{
			changed = (info.lock > 0);
			info.lock = 0;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipKey)
		{
			if(info.type == Game::ChipColor::KEY) {
				info.type = Game::ChipColor::CHIP;
				changed = true;
			}
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipLicorice)
		{
			if(info.type == Game::ChipColor::LICORICE) {
				info.type = Game::ChipColor::CHIP;
				changed = true;
			}
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::MovingMonsterEdit)
		{
			if(info.type == Game::ChipColor::THIEF) {
				info.type = Game::ChipColor::CHIP;
				changed = true;
			}
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipAdapt)
		{
			changed = info.adapt;
			info.adapt = false;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipActions)
		{
			changed = (info.act_count > 0);
			info.act_count = 0;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipTimeBomb)
		{
			changed = (info.time_bomb > 0);
			info.time_bomb = 0;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::DiamondChip && !Core::mainInput.IsControlKeyDown())
		{
			if(info.type == Game::ChipColor::DIAMOND) {
				info.type = Game::ChipColor::CHIP;
				changed = true;
				Log::Info("Diamond deleted");
			}
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipArrows)
		{
			changed = !info.level_hang.IsEmpty();
			info.level_hang.Clear();
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipWithBomb)
		{
			changed = !info.level_hang.IsEmpty();
			info.level_hang.Clear();
		}

		if( changed ) {
			Apply();
		}
		
		return changed;
	}
	
	bool ChipsInfo::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!Game::isSquare(sq) || !sq->IsEmptyForChip())
		{
			return false;
		}
		if(EditorUtils::activeEditBtn == EditorUtils::ChipArrows)
		{
			_chip_arrows_addres = sq->address;
			_chip_arrows_center = GameSettings::CELL_HALF +  GameSettings::ToScreenPos(sq->address.ToPoint()*GameSettings::SQUARE_SIDE);
			FPoint pos = - _chip_arrows_center + mouse_pos;
			float angle = 0;
			if(pos.x != 0 || pos.y != 0)
			{
				angle = pos.GetAngle()*180.0f/math::PI;
				if(angle < 0)
				{
					angle += 360.0f;
				}
			}
			angle = angle + 45.0f;
			_chip_arrows_dir = 2 * (int(angle/90.0f) % 4);
			return true;
		}
		else if( (sq != _prev_square) &&
			 ((EditorUtils::activeEditBtn == EditorUtils::ChipKey)
		      || (EditorUtils::activeEditBtn == EditorUtils::ChipLicorice)
			  || (EditorUtils::activeEditBtn == EditorUtils::ChipAdapt)
			  || (EditorUtils::activeEditBtn == EditorUtils::ChipStar)
			  || (EditorUtils::activeEditBtn == EditorUtils::ChameleonParams)
			  || (EditorUtils::activeEditBtn == EditorUtils::ChipLockEditor))
			  )
		{
			if(Core::mainInput.GetMouseLeftButton())
				return Editor_LeftMouseDown(mouse_pos, sq);
			if(Core::mainInput.GetMouseRightButton())
				return Editor_RightMouseDown(mouse_pos, sq);
		}
		return false;
	}

	bool ChipsInfo::Editor_MouseWheel(int delta, Game::Square *sq)
	{
		if(!sq || delta == 0)
		{
			return false;
		}
		if(EditorUtils::activeEditBtn == EditorUtils::ChipActions)
		{
			_info[sq->address].star = 0;
			_info[sq->address].act_count = math::clamp(0, 1000, _info[sq->address].act_count + delta);
			Apply();
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::ChipTimeBomb)
		{
			_info[sq->address].time_bomb = math::clamp(0, 1000, _info[sq->address].time_bomb + delta);
			Apply();
			return true;
		}
		else  if(EditorUtils::activeEditBtn == EditorUtils::ChipArrows)
		{
			_chip_arrows_length = math::clamp(0, 1000, _chip_arrows_length + delta);
			return true;
		}
		else if(EditorUtils::activeEditBtn == EditorUtils::PreInstalledChip)
		{
			_current_pre_chip = (_current_pre_chip + delta + Gadgets::levelColors.GetCountFull()) % Gadgets::levelColors.GetCountFull();
			return true;
		}
		return false;
	}

	void ChipsInfo::DrawEdit()
	{
		if(EditorUtils::activeEditBtn == EditorUtils::ChipArrows)
		{
			Render::device.SetTexturing(false);
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(math::Vector3(-GameSettings::FIELD_SCREEN_OFFSET));

			//Острие - направление
			math::Vector3 p0, p1;
			p0 = _chip_arrows_center + FPoint(GameSettings::SQUARE_SIDE/2.f, -GameSettings::SQUARE_SIDE/2.f).Rotated(math::PI/2.f*_chip_arrows_dir/2.f); 
			p1 = _chip_arrows_center + FPoint(GameSettings::SQUARE_SIDE/2.f, GameSettings::SQUARE_SIDE/2.f).Rotated(math::PI/2.f*_chip_arrows_dir/2.f);
			Color c = Color(50, 255, 50, 150);
			Render::DrawQuad(_chip_arrows_center, p0, p1, p1, c, c, c, c); 
			
			FPoint offset = GameSettings::ToScreenPos(IPoint(0,0));
			if(_chip_arrows_addres.IsValid() && _info.find(_chip_arrows_addres) != _info.end() && !_info[_chip_arrows_addres].level_hang.IsEmpty())
			{
				Game::Hang &hang = _info[_chip_arrows_addres].level_hang;
				Game::ArrowBonus *arrowHang = (Game::ArrowBonus *)hang.GetBonusByType(Game::HangBonus::ARROW);
				if( arrowHang )
				{
					//Дальность действия текущая
					for(size_t k = 0; k < 4; k++)
					{
						bool draw = false;
						int L = arrowHang->GetRadius();
						if(k*2 == _chip_arrows_dir)
						{
							draw = true;
							Render::BeginColor(Color(255, 50, 50, 150));	
							L = _chip_arrows_length;
						}else if( ((1 << 2*k) & arrowHang->GetDirections()) > 0)
						{
							draw = true;
							Render::BeginColor(c);	
						}
						if(draw)
						{
							for(int i = 0; i < L; i++)
							{
								FPoint dir(1, 0);
								dir.Rotate(math::PI*0.5f*k);
								Game::FieldAddress a = _chip_arrows_addres + Game::FieldAddress((dir*(i+1.0f)).Rounded());
								Game::Square *sq = GameSettings::gamefield[a];
								if(Game::isVisible(sq))
								{
									IRect rect(Game::ChipColor::DRAW_RECT);								
									rect.x = sq->address.ToPoint().x*GameSettings::SQUARE_SIDE + (int)offset.x;
									rect.y = sq->address.ToPoint().y*GameSettings::SQUARE_SIDE + (int)offset.y;
									Render::DrawRect(rect);
								}
							}
							Render::EndColor();
						}
					}
				}
			}
			Render::device.PopMatrix();
			Render::device.SetTexturing(true);	
		} else if(EditorUtils::activeEditBtn == EditorUtils::PreInstalledChip) {
			Game::ChipColor::chipsTex->Bind();
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(Core::mainInput.GetMousePos() - GameSettings::FIELD_SCREEN_OFFSET);
			Render::device.MatrixScale(0.9f);
			Render::BeginAlphaMul(0.8f);
			Render::DrawRect(Game::Square::DRAW_RECT, Game::GetChipRect(Gadgets::levelColors[_current_pre_chip], false, false, false));
			Render::EndAlphaMul();
			Render::device.PopMatrix();
		}
		
		for(std::map<Game::FieldAddress, InfoC>::iterator itr = _info.begin(); itr != _info.end(); ++itr)
		{
			if( itr->second.pre_chip == -2 )
			{
				FPoint pos = FPoint(itr->first.ToPoint()) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
				Render::BindFont("debug");
				Render::PrintString(pos, "chip", 1.0f, CenterAlign, CenterAlign, false);
			}
			if( itr->second.treasure > 0 )
			{
				FPoint pos = FPoint(itr->first.ToPoint()) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
				Render::BindFont("Score");
				Render::PrintString(pos, utils::lexical_cast(itr->second.treasure), 1.0f, CenterAlign, CenterAlign, false);
			}
		}
	}

	bool ChipsInfo::AcceptMessage(const Message &message)
	{
		return false;
	}


	void ChipsInfo::Editor_ClearFieldPart(IRect part)
	{
		std::list<Game::FieldAddress> eraseList;
		for(std::map<Game::FieldAddress, InfoC>::iterator itr = _info.begin(), end = _info.end(); itr != end; ++itr)
		{
			if( part.Contains(itr->first.ToPoint()) )
			{
				eraseList.push_back(itr->first);
			}
		}
		for (std::list<Game::FieldAddress>::iterator itr = eraseList.begin(), end = eraseList.end(); itr != end; ++itr)
		{
			_info.erase(*itr);
		}
	}

	void ChipsInfo::Editor_CopyToClipboard(IRect part)
	{
		Game::FieldAddress offset(part.x, part.y);
		_clipboard.clear();
		for(std::map<Game::FieldAddress, InfoC>::iterator itr = _info.begin(); itr != _info.end(); ++itr)
		{
			if( part.Contains(itr->first.ToPoint()) )
				_clipboard.insert( std::make_pair(itr->first - offset, itr->second) );
		}
	}

	void ChipsInfo::Editor_CutToClipboard(IRect part)
	{
		Editor_CopyToClipboard(part);
		Editor_ClearFieldPart(part);
	}

	bool ChipsInfo::Editor_PasteFromClipboard(IPoint pos)
	{
		Game::FieldAddress offset(pos);
		for(std::map<Game::FieldAddress, InfoC>::iterator itr = _clipboard.begin(); itr != _clipboard.end(); ++itr)
		{
			_info[itr->first + offset] = itr->second;
		}
		_clipboard.clear();
		Apply();
		return true;
	}

}//Gadgets