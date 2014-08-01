#include "StdAfx.h"
#include "FlyingThing.h"
#include "GameField.h"


FlyingThing::FlyingThing(FPoint from, FPoint to, float timeToFly, FlyingThingDrawFunction drawer, FlyingThingFinishFunction finisher, void *finishParams)
	: GameFieldController("FlyingThing", 1.f, GameField::Get())
	, _posFrom(from), _posTo(to), _timeToFly(timeToFly), _drawer(drawer)
	, _finisher(finisher), _finishParams(finishParams)
{
	Init();
}

void FlyingThing::Update(float dt)
{
	local_time += dt;
	_pos += _velocity * dt;

	if (local_time > _timeToFly && _state != FINISHED)
	{
		_state = FINISHED;
		_finisher(_finishParams);
	};
}

bool FlyingThing::isFinish()
{
	return (_state == FINISHED);
}

void FlyingThing::Init()
{
	//считаем скорость
	_velocity = (_posTo - _posFrom) / _timeToFly;

	_pos = _posFrom;

	_state = FLY;
}

void FlyingThing::DrawAbsolute()
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(_pos.x, _pos.y, 0.f));
	_drawer();
	Render::device.PopMatrix();
}
