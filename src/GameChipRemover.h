#ifndef _GAME_CHIP_REMOVER_H_
#define _GAME_CHIP_REMOVER_H_

#include "GameFieldController.h"
#include "GameFieldAddress.h"

namespace Game {

// Риcует иcчезающую фишку
// (фишка в том, что иcчезающая фишка не риcуетcя вмеcте c оcтальными фишками
//	и ее нужно отриcовывать отдельно)
class ChipRemover
	: public GameFieldController
{
	FPoint _pos;
	FRect _uv;
	enum CRState
	{
		CR_DELAY,
		CR_STAY,
		CR_HIDE,
		CR_FINISH,
	}_state;
	float _timeState;
public:
	ChipRemover(FPoint pos, int color, float pause);
	void Update(float dt);
	virtual bool isFinish();
	virtual void Draw();
};

class ChipRemoverByBonus
    : public GameFieldController
{
    FPoint _pos;
    FRect _uv;
    enum CRState
    {
        CR_DELAY,
        CR_STAY,
        CR_HIDE,
        CR_FINISH,
    }_state;
    float _timeState;
public:
    ChipRemoverByBonus(FPoint pos, int color, float pause);
    void Update(float dt);
    virtual bool isFinish();
    virtual void Draw();
};

// Рисует фишку, летящую к замку/приемнику с заказом
class ChipOrderRemover
	: public GameFieldController
{
	SplinePath<FPoint> _path;
	FRect _uv;
	Game::FieldAddress _square;
	int _color;
public:
	ChipOrderRemover(FPoint pos, FPoint to, int color, Game::FieldAddress square);
	void Update(float dt);
	bool isFinish();
	void Draw();
};

} // namespace Game
#endif //_GAME_CHIP_REMOVER_H_