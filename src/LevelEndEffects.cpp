#include "stdafx.h"
#include "LevelEndEffects.h"
#include "CombinedBonus.h"
#include "GameField.h"


LevelEndFlyBonus::LevelEndFlyBonus(GameField *gamefield, Game::FieldAddress to, const Game::Hang& bonus, float delay)
	: GameFieldController("LevelEndFlyBonus", 1.6f, gamefield)
	, _bonus(bonus)
	, _dest(to)
	, _trail(0)
	, _endEffect(0)
	, fromPanel(Core::LuaCallFunction<FPoint>("gameInterfaceCounterPosition"))
{
	_trail = Game::AddEffect(gamefield->_effContUp, "IskraLong");
	Assert(_trail);
	_trail->Reset();

	local_time = -delay;

	FPoint endPos = FPoint(_dest.ToPoint()) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
	endPos = GameSettings::ToScreenPos(endPos);

	FPoint v1 = endPos - fromPanel;
	FPoint v2;
	float t;
	if(v1.y < 1.35f * v1.x) {
		v2 = FPoint(v1.y, -v1.x);
		t = math::random(0.3f, 0.7f);
	} else {
		v2 = FPoint(-v1.y, v1.x);
		t = math::random(0.6f, 0.9f);
	}
	v2.Normalize();
	
	FPoint pt = fromPanel + v1 * t + v2 * v1.Length() * 0.1f;

	_path.addKey(fromPanel);
	_path.addKey(pt);
	_path.addKey(endPos);
	_path.CalculateGradient();
}

LevelEndFlyBonus::~LevelEndFlyBonus()
{
	if(_trail.get())
	{
		_trail->Kill();
		_trail.reset();
	}
}

void LevelEndFlyBonus::Update(float dt)
{
	if( local_time < 0.0f )
	{
		local_time += dt;
	}
	else if( local_time <= 1.0f )
	{
		FPoint pos = _path.getGlobalFrame(local_time);
		_trail->SetPos(pos);

		local_time += time_scale * dt;

		if(local_time > 1.0f)
		{
			_trail->Finish();
			_trail.reset();

			_endEffect = GameField::Get()->_effContUp.AddEffect("BonusAppear");
			_endEffect->SetPos(_path.getGlobalFrame(1.0f));
			_endEffect->Reset();

			// взрываем
			CombinedBonus::BonusCascade cascade;
			cascade.push_back( std::make_pair(_dest, _bonus) );
			Game::AddController(new CombinedBonus(_dest, _bonus, GameField::Get(), false));
		}
	}
}

void LevelEndFlyBonus::DrawAbsolute()
{
}

bool LevelEndFlyBonus::isFinish()
{
	return (local_time > 1.0f) && (!_endEffect.get() || _endEffect->isEnd());
}



LevelEndBonus::LevelEndBonus(float delay, int radius)
	: _delay(delay)
	, _radius(radius)
{
}

void LevelEndBonus::GetAffectedChips(Game::FieldAddress cell, Game::AffectedArea &chips, float startTime)
{
	Game::ClearCellInfo info(startTime + _delay, true);
	HangBonus::AddChip(cell, info, chips);

	for (int i = -_radius; i <= _radius; i++)
	{
		for (int j = _radius; j >= -_radius; j--)
		{
			Game::FieldAddress fa = cell + Game::FieldAddress(i, j);
			Game::Square *sq = GameSettings::gamefield[fa];			
			if( Game::CheckContainInRadius((float)_radius, cell.ToPoint(), fa.ToPoint(), sq) )
			{
				HangBonus::AddChip(fa, info, chips);
			}
		}
	}
}

void LevelEndBonus::StartEffect(Game::FieldAddress from, float startTime)
{
	Game::AddController(new LevelEndBonusController(from.ToPoint(), startTime + _delay));
}

LevelEndBonusController::LevelEndBonusController(IPoint center, float startTime)
	: GameFieldController("LevelEndBonus", 1.0f, GameField::Get())
	, _center(center)
{
	local_time = -startTime;
}

void LevelEndBonusController::Update(float dt)
{
	local_time += dt;
	if( local_time >= 0.0f )
	{
		Game::FieldAddress address(_center);
		FPoint pos = GameSettings::gamefield[address]->GetCellPos() + GameSettings::CELL_HALF;
		ParticleEffectPtr eff = Game::AddEffect(gameField->_effTopCont, "LineBonusChip_0");
		eff->SetPos(pos);
		eff->Reset();

		gameField->addWave(pos.Rounded(), 0.02f, 1000.0f, 125.0f);
	}
}

void LevelEndBonusController::Draw()
{
}

bool LevelEndBonusController::isFinish()
{
	return (local_time > 0.0f);
}



LevelEndEffect::LevelEndEffect()
	: GameFieldController("LevelEndEffect", 0.5f, GameField::Get())
{
	_effect = Game::AddEffect(gameField->_effContUp, "EndLevel");

	_pathX.addKey(0.0f, 210);
	_pathX.addKey(0.15f, 150);
	_pathX.addKey(0.45f, 550);
	_pathX.addKey(0.75f, 210);
	_pathX.addKey(1.0f, 320);
	_pathX.CalculateGradient();

	_pathY.addKey(0.0f, 920);
	_pathY.addKey(0.3f, 180);
	_pathY.addKey(0.6f, 800);
	_pathY.addKey(1.0f, 700);
	_pathY.CalculateGradient();
}

LevelEndEffect::~LevelEndEffect()
{
	if(_effect.get()) {
		_effect->Kill();
		_effect.reset();
	}
}

void LevelEndEffect::Update(float dt)
{
	local_time += time_scale * dt;

	float t = math::clamp(0.0f, 1.0f, local_time);
	//FPoint pos = _path.getGlobalFrame(t);
	float x = _pathX.getGlobalFrame(t);
	float y = _pathY.getGlobalFrame(t);
	_effect->SetPos(x, y);

	if( local_time > 1.0f ) {
		_effect->Finish();
		_effect.reset();

		ParticleEffect *fallEff = gameField->_effContUp.AddEffect("EndLevel_Fall");
		fallEff->SetPos(320.0f, 900.0f);
		fallEff->Reset();

		ParticleEffect *expEff = gameField->_effContUp.AddEffect("EndLevel_Explode");
		expEff->SetPos(320.0f, 700.0f);
		expEff->Reset();

		gameField->TriggerBonusesFromMoves();
	}
}

void LevelEndEffect::Draw()
{
}

bool LevelEndEffect::isFinish()
{
	return (local_time > 1.0f);
}