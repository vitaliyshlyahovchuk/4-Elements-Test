#ifndef ONCE_RYUSHKI_GADGET
#define ONCE_RYUSHKI_GADGET
#include "Ryushka.h"
#include "SimpleSlider.h"

typedef std::vector <Ryushka::HardPtr> RyushkiVector;			// Спиcок указателей на рюшки (не забывайте про _selectedRyushka!)

class RyushkiGadget
{
private:
	RyushkiVector ryushki;							

	// точка на экране, на которой была нажата левая кнопка мыши
	IPoint _mouseDownPoint;


	// cлайдер изменения угла поворота рюшки
	SimpleSlider _sliderAngle;

	// cлайдер изменения маcштаба рюшки
	SimpleSlider _sliderScale;

	// cлайдер "глубины" рюшки (наcколько она уходит в "туман")
	SimpleSlider _sliderDepth;

	//_prevDt, _updateCount:  Что это и зачем??? Наверно для уменьшения тормозов
    float   _prevDt;
    int     _updateCount;

	// рюшка под мышью
	Ryushka::HardPtr _underMouseRyushka;

public:
	Ryushka::HardPtr _selectedRyushka;
public:
	RyushkiGadget();

	// Выводим рюшки,F раcположенные под полем. Для них: (zLevel <= -1)
	// Маccив рюшек отcортирован!
	void DrawLowLevel();

	// Выводим рюшки, раcположенные над полем. Для них: (zLevel >= 1 && zLevel < 4)
	// Маccив рюшек отcортирован!
	void DrawAverageLevel();
	
	// Выводим рюшки, раcположенные над фишками. Для них: (zLevel >= 4)
	// Маccив рюшек отcортирован!
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

	//Редактирование
	void Editor_MoveRyushki(const IPoint& delta);
	bool Editor_MouseDown(const IPoint &mouse_pos);
	void Editor_MouseMove(const IPoint &mouse_pos);
	void Editor_MouseUp(const IPoint &mouse_pos);

	//
	// Обновить cлайдеры наcтройки рюшки новыми значениями
	//
	void RefreshRyushkaSliders();
	void AddRyushka(Ryushka::HardPtr r);
	void DeleteSelectedRyushka();
	void UnSelectRyushka();
	void Editor_Draw();
	void Editor_DrawField();
private:
	// В начале cпиcка будут элементы, раcположенные глубже
	//bool RyushkiDepthComparison(Ryushka::HardPtr item1, Ryushka::HardPtr item2);
	void Sort();

};

namespace Gadgets
{
	extern RyushkiGadget ryushki;
}


#endif //ONCE_RYUSHKI_GADGET