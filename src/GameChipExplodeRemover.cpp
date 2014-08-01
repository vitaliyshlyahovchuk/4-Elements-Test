#include "stdafx.h"
#include "GameChipExplodeRemover.h"
#include "GameField.h"
#include "Game.h"

namespace Game {

	ChipExplodeRemover::ChipExplodeRemover(int color, const Game::FieldAddress &address_chip, FPoint pos, float dx, float dy, GameField *gamefield_)
	: GameFieldController("ChipExplodeRemover", 0.6f, gamefield_)
	, _pos(pos)
	, _dx(dx)
	, _dy(dy)
	, _angle(0.f)
	, _zTime(0.f)
{
	_dx *= 1.8f;
	_dy *= 2.f;
	_dy += 250.f;

	_dx += math::random(-30.f, 30.f);

	_dAngle = math::random(-20.f, 20.f) - dx*2.f;

	local_time = 0.f;
	_uv = Game::GetChipRect(color, false, false, false);
}

void ChipExplodeRemover::Update(float dt)
{
	local_time += dt*time_scale;

	_pos.x += _dx*dt;
	_pos.y += _dy*dt;
	_dy -= 1000.f*dt;

	_angle += dt*_dAngle;

	_zTime += dt*1.3f;
	if (_zTime > 1.f)
	{
		_zTime = 1.f;
	}
}

bool ChipExplodeRemover::isFinish()
{
	return (local_time >= 1.f);
}

void ChipExplodeRemover::Draw()
{
	Render::device.PushMatrix();

	Render::device.MatrixTranslate(_pos);

	Render::BeginAlphaMul(math::clamp(0.f, 1.f, 1 - local_time));

	float scale = 1.f + 0.1f*math::sin(_zTime*math::PI/2);
	Render::device.MatrixScale(scale);

	Render::device.MatrixRotate(math::Vector3::UnitZ, _angle);

	ChipColor::chipsTex->Draw(ChipColor::DRAW_FRECT.MovedBy(-GameSettings::CELL_HALF), _uv);

	Render::device.PopMatrix();

	Render::EndAlphaMul();
}

} // namespace Game