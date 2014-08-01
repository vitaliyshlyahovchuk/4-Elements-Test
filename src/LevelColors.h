#pragma once
#include "types.h"

// ������ ���������� �� ���������� ������ �����, ������ ��� - ���� 1, 2�� ��� - ���� 2 � �.�.
typedef DWORD ColorMask;
ColorMask ColorToMask(int color);
bool ColorInMask(ColorMask mask, int color);

class LevelColors
{
	int FULL_COUNT;		// ������������ ���������� ���������� �����
	int _nColors;		// ������c��� ������ ����� � ������
	int _colors[32];	// ����� �����
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