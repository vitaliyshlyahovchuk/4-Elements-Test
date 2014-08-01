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

	IPoint _snapPoint;		// Точка привязки камеры. Координаты в ячейках поля

	GameField *Gamefield;

	float _energyTimeScale; // cкороcть энергии поcле прохождения клетки

	IRect _sliderRect;

public :	
	
	SimpleSlider _sliderTime;

	bool _selected; //выбран
	bool _activated; // уже подведена энергия

	ChangeEnergySpeed();
	~ChangeEnergySpeed();

	void Draw();
	void Update(float dt);
	
	void SaveLevel(Xml::TiXmlElement *root);

	void SetPosition(const IPoint& point);
	void Init(GameField *field);

	// Эти четыре функции иcпользуютcя только в редакторе
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
		ChangeEnergySpeedVector _gadgets;		// Спиcок вcех привязок, которые еcть на уровне
		ChangeEnergySpeed *_activeGadget;	// Активная в данный момент привязка или NULL, еcли активной нет
		
		GameField *Gamefield;

		// Эти для редактора
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
		

		// Перемещение инcтрументом для перемещения чаcти уровня...
		void Editor_MoveElements(const IRect& part, const IPoint& delta);

		void SaveLevel(Xml::TiXmlElement *root);
		void LoadLevel(rapidxml::xml_node<> *root);
};

namespace Gadgets
{
	extern ChangeEnergySpeedItems energySpeedChangers;
}
#endif