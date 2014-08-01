#ifndef _FREE_FRONT_H_
#define _FREE_FRONT_H_
#include "Game.h"


struct ExhaustiveRoundWayElement
{
	//Game::FieldAddress a;
	Game::FieldAddress a_checking;
	bool border;
	size_t debug_dir; //Направление, нужно только для дебажной отрисовки по сути
	ExhaustiveRoundWayElement()
		: border(false)
	{
	
	}
	void Init(const Game::FieldAddress &new_a, const size_t &dir, const bool &use_current_addres);
	Game::FieldAddress GetChildAddres();
	void DrawEdit();
};

class FreeFrontDetector
{
public:
	//Фронт. Содержит элементы - связи между двумя клетками.
	std::vector<ExhaustiveRoundWayElement> _freeFrontForEnergy;
	bool _cameraIsStand;
public:
	void DrawEdit();
	void LoadLevel();

	//Рассчитываются все клетки куда может притечь энергия
	void Update();

	//use_current_addres - для порталов нужно использовать тот же самый адрес
	void GrowFront(ExhaustiveRoundWayElement &element, const bool &use_current_addres);
	bool CameraIsStand();
	void SetCameraIsStand();
};


namespace Gadgets
{
	// матрица раccтояний клеток от приёмника для cтрелки приёмника
	extern Array2D<float> squareDistRec;  //минимальное расстояние до ресивера!
	extern Array2D<float> squareDist;	  //(максимум в источнике, затем по убывающей во все стороны

	void FreeFrontInit();
	void InitProcessSettings();

	//Будем использовать фронт энергии для рассчета условия падать летающему духу на клетку или висеть и ждать пока энергия дотечет и поле долетит до новой точки
	extern FreeFrontDetector freeFrontDetector;

} //Gadgets

#endif //_FREE_FRONT_H_