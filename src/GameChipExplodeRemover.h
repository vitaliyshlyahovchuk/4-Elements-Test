#pragma once
#include "GameFieldController.h"
#include "GameFieldAddress.h"

namespace Game {

struct Square;

//
// Клаcc взрывающихcя фишек
//
class ChipExplodeRemover
	: public GameFieldController
{
	
	FPoint _pos;

	float _dx, _dy;

	FRect _uv;

	float _angle;

	float _dAngle;

	float _zTime;

public:

	ChipExplodeRemover(int color, const Game::FieldAddress &address_chip, FPoint pos, float dx, float dy, GameField *gamefield_);

	void Update(float dt);

	virtual bool isFinish();

	virtual void Draw();
};

} // namespace Game
