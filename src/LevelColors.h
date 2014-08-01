#pragma once
#include "types.h"

// хранит комбинацию из нескольких цветов фишек, первый бит - цвет 1, 2ой бит - цвет 2 и т.д.
typedef DWORD ColorMask;
ColorMask ColorToMask(int color);
bool ColorInMask(ColorMask mask, int color);

class LevelColors
{
	int FULL_COUNT;		// максимальное количество уникальных фишек
	int _nColors;		// количеcтво цветов фишек в уровне
	int _colors[32];	// цвета фишек
public:
	LevelColors();
	void Load(rapidxml::xml_node<> *xml_elem);
	void Save(Xml::TiXmlElement *xml_level);
	void ApplyColors(std::vector<int> &vec);
	int GetRandom() const;
	int GetCount() const;
	int GetCountFull() const;
	int operator[](const int &index) const;
	int GetIndex(int color) const;

	ColorMask GetAllColors() const;
};

namespace Gadgets
{
	extern LevelColors levelColors;
}