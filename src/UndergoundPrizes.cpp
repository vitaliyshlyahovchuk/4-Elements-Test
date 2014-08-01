#include "stdafx.h"
#include "UndergoundPrizes.h"
#include "EditorUtils.h"
#include "GameField.h"

namespace Gadgets
{
	UgPriseInfo::UgPriseInfo(const std::string &type, const IPoint &size)
		: _type(type)
		, _size(size)
		, _time_activate(0.f)
		, _time_activate_scale(1.f)
		, _activated(false)
	{
	}

	void UgPriseInfo::SetAddress(const Game::FieldAddress &address)
	{
		_address = address;
		_pos = _address.ToPoint()*GameSettings::SQUARE_SIDE;
		//Если все в порядке, нужно проверить землю под новоиспеченным бонусом
		for(int x = 0; x < _size.x; x++)
		{
			for(int y = 0; y < _size.y; y++)
			{
				Game::Square* sq = GameSettings::gamefield[_address + Game::FieldAddress(x, y)];
				if(sq->GetWall() == 0)
				{
					sq->SetWall(1);
				}
			}
		}
	}

	void UgPriseInfo::LoadLevel(rapidxml::xml_node<> *xml_element)
	{
		SetAddress(xml_element);
	}

	void UgPriseInfo::SaveLevel(Xml::TiXmlElement *xml_element)
	{
		xml_element->SetAttribute("type", _type.c_str());
		_address.SaveToXml(xml_element);
	}

	bool UgPriseInfo::Update(float dt)
	{
		for(int x = 0; x < _size.x; x++)
		{
			for(int y = 0; y < _size.y; y++)
			{
				Game::Square* sq = GameSettings::gamefield[_address + Game::FieldAddress(x, y)];
				if(sq->GetWall() != 0)
				{
					return false;
				}
			}
		}
		return true;
	};

	//UgPriseBomb


	UgPriseBomb::UgPriseBomb()
		: UgPriseInfo("Bomb")
	{
		_texture = Core::resourceManager.Get<Render::Texture>("UgPrizeBomb");
		_time_activate_scale = 4.f;
	}


	bool UgPriseBomb::Update(float dt)
	{
		if(!_activated)
		{
			if(UgPriseInfo::Update(dt))
			{
				_activated = true;
				GameField::Get()->DoBigBomb(_address.Get(), 2);
			}
		}
		if(_activated)
		{
			_time_activate += dt*_time_activate_scale;
			return _time_activate >= 1.f;
		}
		return false;
	}

	void UgPriseBomb::Draw()
	{
		_texture->Draw(_pos);
	}

	//*******************************
	//UgPriseSun
	//*******************************

	UgPriseSun::UgPriseSun()
		: UgPriseInfo("Sun")
	{
		_texture = Core::resourceManager.Get<Render::Texture>("UgPrizeSun");
		_time_activate_scale = 4.f;
	}


	bool UgPriseSun::Update(float dt)
	{
		if(!_activated)
		{
			if(UgPriseInfo::Update(dt))
			{
				_activated = true;
			}
		}
		if(_activated)
		{
			_time_activate += dt*_time_activate_scale;
			return _time_activate >= 1.f;
		}
		return false;
	}

	void UgPriseSun::Draw()
	{
		_texture->Draw(_pos);
	}

	//
	void UgPriseMaker::Init(GameField *field, const bool &with_editor)
	{
		BaseEditorMaker::Init(field, with_editor);
		_current_type = "Bomb";
		current_element.reset();
	}

	UgPriseInfo::HardPtr UgPriseMaker::CreateElement(const std::string &type)
	{
		if(type == "Bomb")
		{
			return UgPriseInfo::HardPtr(new UgPriseBomb());
		}
		Assert(false);
		return UgPriseInfo::HardPtr();
	}

	void UgPriseMaker::LoadLevel(rapidxml::xml_node<> *root)
	{
		Clear();
		rapidxml::xml_node<> *xml_prizes = root->first_node("Prizes");
		if(!xml_prizes)
		{
			return;
		}
		rapidxml::xml_node<> *xml_element = xml_prizes->first_node("element");
		while(xml_element)
		{
			UgPriseInfo::HardPtr next_element = UgPriseMaker::CreateElement(Xml::GetStringAttribute(xml_element, "type"));
			if(next_element)
			{
				next_element->LoadLevel(xml_element);
				_elements.push_back(next_element); 
			}
			xml_element = xml_element->next_sibling("element");
		}
	}

	void UgPriseMaker::SaveLevel(Xml::TiXmlElement *root)
	{
		if(_elements.empty())
		{
			return;
		}
		Xml::TiXmlElement *xml_prizes = root -> InsertEndChild(Xml::TiXmlElement("Prizes")) -> ToElement();
		for(size_t i = 0; i < _elements.size();i++)
		{
			Xml::TiXmlElement *xml_element = xml_prizes -> InsertEndChild(Xml::TiXmlElement("element")) -> ToElement();
			_elements[i]->SaveLevel(xml_element);
		}
	}

	void UgPriseMaker::DrawUnderWalls()
	{
		for(size_t i = 0; i < _elements.size();i++)
		{
			_elements[i]->Draw();
		}
	}

	void UgPriseMaker::DrawEdit()
	{
		if(EditorUtils::activeEditBtn != EditorUtils::UndergroundPrize)
		{
			Render::BeginAlphaMul(0.5f);
		}
		DrawUnderWalls();
		if(EditorUtils::activeEditBtn != EditorUtils::UndergroundPrize)
		{
			Render::EndAlphaMul();
		}
	}


	void UgPriseMaker::Apply()
	{
	}

	void UgPriseMaker::Clear()
	{
		_elements.clear();
	}

	void UgPriseMaker::Release()
	{
		Clear();
	}

	UgPriseInfo::HardPtr UgPriseMaker::FindElement(const Game::FieldAddress &need_address)
	{
		for(size_t i = 0; i < _elements.size();i++)
		{
			if(_elements[i]->Contains(need_address))
			{
				return _elements[i];
			}
		}
		return UgPriseInfo::HardPtr();
	}

	bool UgPriseMaker::CheckCrosses(UgPriseInfo::HardPtr checked_element, const Game::FieldAddress &checked_a)
	{
		size_t x = 0; 
		size_t y = 0;
		//for(size_t x = 0; x < cheked_element->_size.x; x++)
		//{
		//	for(size_t y = 0; y < cheked_element->_size.y; y++)
		//	{
				Game::FieldAddress a = checked_a + Game::FieldAddress(x,y);
				if(!Game::isVisible(a))
				{
					return false;
				}
				//Пройдемся по всем клеткам проверяемого, проверим с каждым из списка
				for(size_t i = 0; i < _elements.size();i++)
				{
					if(checked_element != _elements[i] && _elements[i]->Contains(a))
					{
						return false;
					}
				}
		//	}
		//}
		return true;	
	}

	bool UgPriseMaker::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(!Game::isVisible(sq) || EditorUtils::activeEditBtn != EditorUtils::UndergroundPrize)
		{
			return false;
		}
		current_element = FindElement(sq->address);
		if(current_element)
		{
			_offset_down = sq->address - current_element->_address;
			return true;	
		}
		UgPriseInfo::HardPtr element_new = UgPriseMaker::CreateElement(_current_type);
		if(element_new)
		{
			if(!CheckCrosses(element_new, sq->address))
			{
				return false;
			}
			//Элемент создан успешно
			element_new->SetAddress(sq->address);
			_elements.push_back(element_new);
			return true;
		}
		return false;
	}

	bool UgPriseMaker::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(current_element)
		{
			return true;
		}
		current_element = FindElement(sq->address);
		if(current_element)
		{
			return RemoveElement(current_element);
		}
		return false;
	}

	bool UgPriseMaker::RemoveElement(UgPriseInfo::HardPtr item)
	{
		std::vector<UgPriseInfo::HardPtr>::iterator it = std::find(_elements.begin(), _elements.end(), item);
		if( it != _elements.end() ){
			_elements.erase(it);
			return true;
		}
		return false;
	}

	bool UgPriseMaker::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(EditorUtils::activeEditBtn != EditorUtils::UndergroundPrize)
		{
			return false;
		}else{
			if(!current_element)
			{
				return false;
			}
			if(CheckCrosses(current_element, sq->address - _offset_down))
			{
				current_element->SetAddress(sq->address - _offset_down);
			}
			//else{
			//	return RemoveElement(current_element);	
			//}
		}
		current_element.reset();
		return false;
	}

	bool UgPriseMaker::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
	{
		if(current_element != NULL)
		{
			if(CheckCrosses(current_element, sq->address  - _offset_down))
			{
				current_element->SetAddress(sq->address - _offset_down);
			}
			return true;
		}
		return false;
	}

	void UgPriseMaker::Editor_CutToClipboard(IRect part)
	{
		_clipboard.clear();
		Game::FieldAddress offset(-part.LeftBottom());
		for(std::vector<UgPriseInfo::HardPtr>::iterator itr = _elements.begin(); itr != _elements.end(); )
		{
			if( part.Contains((*itr)->GetRect()) ){
				(*itr)->_address += offset;
				_clipboard.push_back(*itr);
				itr = _elements.erase(itr);
			} else {
				++itr;
			}
		}
	}

	bool UgPriseMaker::Editor_PasteFromClipboard(IPoint pos)
	{
		Game::FieldAddress offset(pos);
		for(std::vector<UgPriseInfo::HardPtr>::iterator itr = _clipboard.begin(); itr != _clipboard.end(); ++itr)
		{
			(*itr)->SetAddress((*itr)->_address + offset);
			_elements.push_back(*itr);
		}
		_clipboard.clear();
		return true;
	}

	bool UgPriseMaker::Editor_MouseWheel(int delta, Game::Square *sq)
	{
		return false;
	}


	bool UgPriseMaker::AcceptMessage(const Message &message)
	{
		return false;
	}

	void UgPriseMaker::Update(float dt)
	{
		std::vector<UgPriseInfo::HardPtr>::iterator it = _elements.begin();
		while(it != _elements.end())
		{
			if( (*it)->Update(dt))
			{
				it = _elements.erase(it);
			}else{
				it++;
			}
		}
	}

}//namespace Gadgets

