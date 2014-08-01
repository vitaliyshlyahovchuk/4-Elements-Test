#pragma once

class GameField;

enum DrawTypes
{
	GDP_DOWN,
	GDP_UP,
	GDP_SMOOTH,
};


//
// ������� ���cc ��� ������������,
// ������� c����� ������� ����
//
class GameFieldController
	: public IController	
{

public:

	float time_scale;

	GameField *gameField;

	int z;

	// ��c���c� � ������� ����������� ��� �������
	virtual void Draw() {};

	// ��c���c� � ������� ����������� ��� �������
	virtual void DrawUnderChips(){};

	// ��c���c� � �������� ����������� ��� ������ �c��
	virtual void DrawAbsolute(){};

	GameFieldController(const std::string  &name, float time_scale_, GameField *gamefield);

	virtual ~GameFieldController();

	virtual bool MouseDown(const IPoint &mouse_pos){return false;}
	virtual bool MouseMove(const IPoint &mouse_pos){return false;}
	virtual bool MouseUp(const IPoint &mouse_pos){return false;}

};
