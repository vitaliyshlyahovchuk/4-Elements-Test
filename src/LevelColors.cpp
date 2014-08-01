#include "stdafx.h"
#include "LevelColors.h"
#include "GameInfo.h"
#include "Game.h"

namespace Gadgets
{
	LevelColors levelColors;
}

ColorMask ColorToMask(int color)
{
	return (1u << color);
}

bool ColorInMask(ColorMask mask, int color)
{
	return ((1u << color) & mask) > 0;
}

LevelColors::LevelColors()
	: _nColors(0)
	, FULL_COUNT(6)
{
}

void LevelColors::Load(rapidxml::xml_node<> *xml_level)
{
	_nColors = 0;
	rapidxml::xml_node<> *xml_chips = xml_level->first_node("Chips");
	if(xml_chips)
	{
		rapidxml::xml_node<> *xml_chip = xml_chips->first_node("Chip");
		while (xml_chip)
		{
			int value = utils::lexical_cast<int>(xml_chip->last_attribute("n")->value());
			MyAssert(0 < value && value <= FULL_COUNT);
			_colors[_nColors] = value;
			_nColors++;
			xml_chip = xml_chip->next_sibling("Chip");
		}
	}
	//Сверху добавляем все не попавшие в список генерируемых на уровне фишек.
	//Сделано для того чтобы их можно было явно предварительно устанавливать на уровне.
	int count = _nColors;
	for(int i = 1; i <= FULL_COUNT; ++i)
	{
		bool find = false;
		for(int k = 0; k < _nColors; ++k)
		{
			if(_colors[k] == i)
			{
				find = true;
				break;
			}
		}
		if(!find)
		{
			_colors[count] = i;		
			count++;
		}
	}
	Assert2(_nColors > 0, "No chips are chosen for level");
}

void LevelColors::ApplyColors(std::vector<int> &vec)
{
	size_t count = vec.size();
	_nColors = count;
	for(size_t i = 0; i < count; i++)
	{
		MyAssert(vec[i] < 32);
		_colors[i] = vec[i];
	}
}

void LevelColors::Save(Xml::TiXmlElement *xml_level)
{
	Xml::TiXmlElement *xml_chips = xml_level->InsertEndChild(Xml::TiXmlElement("Chips"))->ToElement();
	for (int q = 0; q <_nColors; q++)
	{
		Xml::TiXmlElement *xml_chip = xml_chips->InsertEndChild(Xml::TiXmlElement("Chip"))->ToElement();
		xml_chip->SetAttribute("n", utils::lexical_cast(_colors[q]));
	}
}

int LevelColors::GetRandom() const
{
	MyAssert(_nColors > 0);
	return _colors[math::random(0, _nColors-1)];
}

int LevelColors::GetCount() const
{
	return _nColors;
}

int LevelColors::GetCountFull() const
{
	return FULL_COUNT;
}

int LevelColors::operator[](const int &index) const
{
	if(index >= GetCountFull())
	{		
		return _colors[_nColors-1];
	}
	return _colors[index];
}

int LevelColors::GetIndex(int color) const
{
	for(int i = 0; i < _nColors; i++)
	{
		if(_colors[i] == color)
		{
			return i;
		}
	}
	return -1;
}

ColorMask LevelColors::GetAllColors() const
{
	ColorMask result = 0;
	for(int i = 0; i < _nColors; i++)
	{
		result |= ColorToMask(_colors[i]);
	}
	return result;
}

