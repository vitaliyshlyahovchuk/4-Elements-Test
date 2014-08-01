#ifndef ONCE_RYUSHKI_GADGET
#define ONCE_RYUSHKI_GADGET
#include "Ryushka.h"
#include "SimpleSlider.h"

typedef std::vector <Ryushka::HardPtr> RyushkiVector;			// ���c�� ���������� �� ����� (�� ��������� ��� _selectedRyushka!)

class RyushkiGadget
{
private:
	RyushkiVector ryushki;							

	// ����� �� ������, �� ������� ���� ������ ����� ������ ����
	IPoint _mouseDownPoint;


	// c������ ��������� ���� �������� �����
	SimpleSlider _sliderAngle;

	// c������ ��������� ��c����� �����
	SimpleSlider _sliderScale;

	// c������ "�������" ����� (��c������ ��� ������ � "�����")
	SimpleSlider _sliderDepth;

	//_prevDt, _updateCount:  ��� ��� � �����??? ������� ��� ���������� ��������
    float   _prevDt;
    int     _updateCount;

	// ����� ��� �����
	Ryushka::HardPtr _underMouseRyushka;

public:
	Ryushka::HardPtr _selectedRyushka;
public:
	RyushkiGadget();

	// ������� �����,F ��c���������� ��� �����. ��� ���: (zLevel <= -1)
	// ��cc�� ����� ��c���������!
	void DrawLowLevel();

	// ������� �����, ��c���������� ��� �����. ��� ���: (zLevel >= 1 && zLevel < 4)
	// ��cc�� ����� ��c���������!
	void DrawAverageLevel();
	
	// ������� �����, ��c���������� ��� �������. ��� ���: (zLevel >= 4)
	// ��cc�� ����� ��c���������!
	void DrawHighLevel();

	void Update(float dt);

	bool MouseDown(const IPoint &mouse_pos);
	void MouseMove(const IPoint &mouse_pos);

	void ClearRyushki();
	void LoadRyushki(rapidxml::xml_node<>* root);
	void SaveLevel(Xml::TiXmlElement *xml_level);
	void Reset();
	void AcceptMessage(const Message &message);
	Message QueryState(const Message &message) const;

	//��������������
	void Editor_MoveRyushki(const IPoint& delta);
	bool Editor_MouseDown(const IPoint &mouse_pos);
	void Editor_MouseMove(const IPoint &mouse_pos);
	void Editor_MouseUp(const IPoint &mouse_pos);

	//
	// �������� c������� ��c������ ����� ������ ����������
	//
	void RefreshRyushkaSliders();
	void AddRyushka(Ryushka::HardPtr r);
	void DeleteSelectedRyushka();
	void UnSelectRyushka();
	void Editor_Draw();
	void Editor_DrawField();
private:
	// � ������ c��c�� ����� ��������, ��c���������� ������
	//bool RyushkiDepthComparison(Ryushka::HardPtr item1, Ryushka::HardPtr item2);
	void Sort();

};

namespace Gadgets
{
	extern RyushkiGadget ryushki;
}


#endif //ONCE_RYUSHKI_GADGET