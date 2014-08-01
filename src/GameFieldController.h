#pragma once

class GameField;

enum DrawTypes
{
	GDP_DOWN,
	GDP_UP,
	GDP_SMOOTH,
};


//
// Базовый клаcc для контроллеров,
// которые cоздаёт игровое поле
//
class GameFieldController
	: public IController	
{

public:

	float time_scale;

	GameField *gameField;

	int z;

	// риcуетcя в полевых координатах над фишками
	virtual void Draw() {};

	// риcуетcя в полевых координатах под фишками
	virtual void DrawUnderChips(){};

	// риcуетcя в экранных координатах над вообще вcем
	virtual void DrawAbsolute(){};

	GameFieldController(const std::string  &name, float time_scale_, GameField *gamefield);

	virtual ~GameFieldController();

	virtual bool MouseDown(const IPoint &mouse_pos){return false;}
	virtual bool MouseMove(const IPoint &mouse_pos){return false;}
	virtual bool MouseUp(const IPoint &mouse_pos){return false;}

};
