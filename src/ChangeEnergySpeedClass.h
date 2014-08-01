#ifndef ONCE_CHANGE_SPEED_CLASS
#define ONCE_CHANGE_SPEED_CLASS

#include "SimpleSlider.h"

class GameField;

namespace Game {
	struct Square;
}

class ChangeEnergySpeed
{
private :

	IPoint _snapPoint;		// ����� �������� ������. ���������� � ������� ����

	GameField *Gamefield;

	float _energyTimeScale; // c����c�� ������� ��c�� ����������� ������

	IRect _sliderRect;

public :	
	
	SimpleSlider _sliderTime;

	bool _selected; //������
	bool _activated; // ��� ��������� �������

	ChangeEnergySpeed();
	~ChangeEnergySpeed();

	void Draw();
	void Update(float dt);
	
	void SaveLevel(Xml::TiXmlElement *root);

	void SetPosition(const IPoint& point);
	void Init(GameField *field);

	// ��� ������ ������� �c��������c� ������ � ���������
	void Editor_MoveElements(const IRect& part, const IPoint& delta);
	void Editor_MoveGadget(const IPoint& mouse_pos, int x, int y);
	int Editor_CaptureGadget(const IPoint& mouse_pos, int x, int y);
	void Editor_ReleaseGadget(const IPoint& mouse_pos, int x, int y);

	void SetEnergyTimeScale(float scale)
	{
		_energyTimeScale = scale;
	}
};

typedef std::vector <ChangeEnergySpeed *> ChangeEnergySpeedVector;

class ChangeEnergySpeedItems
{
	private :
		ChangeEnergySpeedVector _gadgets;		// ���c�� �c�� ��������, ������� �c�� �� ������
		ChangeEnergySpeed *_activeGadget;	// �������� � ������ ������ �������� ��� NULL, �c�� �������� ���
		
		GameField *Gamefield;

		// ��� ��� ���������
		ChangeEnergySpeed *_dragGadget;
		ChangeEnergySpeed *_selectedGadget;
		
	public :
		ChangeEnergySpeedItems();
		~ChangeEnergySpeedItems();
		void Release();

		void Reset();
		void Clear();

		void Update(float dt);

		void AddGadget(ChangeEnergySpeed *gadget);
		void Init(GameField *field);
		
		const ChangeEnergySpeed *GetActiveGadget() 
		{ 
			return (_activeGadget); 
		}

		void Editor_Draw();
		void Editor_MoveItem(const IPoint& mouse_pos, int x, int y);
		bool Editor_CaptureItem(const IPoint& mouse_pos, int x, int y);
		bool Editor_CheckRemove(const IPoint& mouse_pos, int x, int y);
		void Editor_ReleaseItem();
		bool Editor_RemoveUnderMouse(const IPoint& mouse_pos, int x, int y);

		void Editor_MouseDown(const IPoint& mouse_pos);
		void Editor_MouseUp(const IPoint& mouse_pos);
		void Editor_MouseMove(const IPoint& mouse_pos);
		

		// ����������� ��c��������� ��� ����������� ��c�� ������...
		void Editor_MoveElements(const IRect& part, const IPoint& delta);

		void SaveLevel(Xml::TiXmlElement *root);
		void LoadLevel(rapidxml::xml_node<> *root);
};

namespace Gadgets
{
	extern ChangeEnergySpeedItems energySpeedChangers;
}
#endif