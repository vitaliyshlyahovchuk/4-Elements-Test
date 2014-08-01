#include "stdafx.h"

#include "CombinedBonus.h"
#include "Game.h"
#include "GameField.h"
#include "Match3.h"
#include "GameLightningController.h"
#include "BombField.h"

CombinedBonus::CombinedBonus(Game::FieldAddress cell, Game::Hang bonus, GameField *field, bool endMove)
	: GameFieldController("CombinedBonus", 1.0f, field)
	, _totalDuration(0.0f)
	, _colors(0)
	, _endMove(endMove)
{
	BonusCascade bonuses;
	bonuses.push_back( std::make_pair(cell, bonus) );

	z = 3;
	Init(bonuses, INSTANT);
}

CombinedBonus::CombinedBonus(BonusCascade& bonuses, GameField *field, bool endMove, TriggerTiming timing)
	: GameFieldController("CombinedBonus", 1.0f, field)
	, _totalDuration(0.0f)
	, _colors(0)
	, _endMove(endMove)
{
	z = 3;
	Init(bonuses, timing);
}

void CombinedBonus::TriggerBonus(const Game::FieldAddress& fa, Game::Hang& hang, float startTime)
{
	hang.GetAffectedChips(fa, _chips, startTime);

	hang.StartEffects(fa, startTime);

	_colors |= hang.GetColor();

	Gadgets::bombFields.StartBang(fa.ToPoint(), false);

	Game::Square *sq = GameSettings::gamefield[fa];
	int chipColor = sq->GetChip().GetColor();

	int score = gameField->_endLevel.IsRunning() ? GameSettings::score.bonus[1] : GameSettings::score.bonus[0];
	gameField->AddScore(fa, score, startTime, 1.2f, (chipColor >= 0) ? GameSettings::chip_settings[chipColor].color_score : Color::WHITE);
}

void CombinedBonus::Init(BonusCascade& bonuses, TriggerTiming timing)
{
	if( timing == INSTANT )
	{
		for(BonusCascade::iterator itr = bonuses.begin(); itr != bonuses.end(); itr++)
		{
			TriggerBonus(itr->first, itr->second, 0.0f);	
		}
	}
	else if( timing == RANDOM )
	{
		if( !bonuses.empty() )
		{
			std::random_shuffle(bonuses.begin(), bonuses.end());
			float delay = 0.0f;
			float dt = std::max(1.2f / bonuses.size(), 0.1f);
			for(BonusCascade::iterator itr = bonuses.begin(); itr != bonuses.end(); itr++)
			{
				TriggerBonus(itr->first, itr->second, delay);
				delay += dt;
			}
		}
	}
	else if( timing == CHAIN )
	{
		std::list< std::pair<Game::FieldAddress, Game::Hang> > bonusList;
		bonusList.assign( bonuses.begin(), bonuses.end() );

		while( !bonusList.empty() )
		{
			std::pair<Game::FieldAddress, Game::Hang> &bonus = bonusList.front();
			TriggerBonus(bonus.first, bonus.second, 0.0f);
			bonusList.pop_front();

			bool triggered;

			do
			{
				triggered = false;
				for(auto itr = bonusList.begin(); itr != bonusList.end(); )
				{
					std::pair<Game::FieldAddress, Game::Hang> &bonus = bonusList.front();
				
					Game::AffectedArea::iterator chips_itr = _chips.find(bonus.first);
					if( chips_itr != _chips.end() ){
						TriggerBonus(bonus.first, bonus.second, chips_itr->second.delay);
						itr = bonusList.erase(itr);
						triggered = true;
					} else {
						++itr;
					}
				}
			}
			while(triggered);
		}
	}
	else
	{
		Assert(false);
	}

	for(Game::AffectedArea::const_iterator itr = _chips.begin(); itr != _chips.end(); ++itr)
	{
		if( itr->second.delay > _totalDuration )
			_totalDuration = itr->second.delay;
	}

	if(!_chips.empty())
		_totalDuration += 0.5f;
}

void CombinedBonus::Update(float dt)
{
	local_time += time_scale * dt;
	DestroyChips();
}

void CombinedBonus::DestroyChips()
{
	for(Game::AffectedArea::iterator itr = _chips.begin(); itr != _chips.end();  )
	{
		if( itr->second.delay <= local_time )
		{
			Game::Square *sq = GameSettings::gamefield[itr->first];

			sq->GetChip().GetHang().Clear();
			Game::ClearCell(sq, sq, Game::GetCenterPosition(sq->address), true, 0.0f, !itr->second.kill_near, _colors, GameSettings::score.chip_b);
			_fallColumns.insert(itr->first.GetCol());

			itr = _chips.erase(itr);
		}
		else
		{
			++itr;
		}
	}
}

void CombinedBonus::RunFallColumns()
{
	for(int col : _fallColumns)
	{
		Match3::RunFallColumn(col);
	}
	_fallColumns.clear();
}

bool CombinedBonus::isFinish()
{
	bool isFinished = (local_time > _totalDuration);

	if(isFinished)
	{
		RunFallColumns();
		if(_endMove)
			gameField->EndMove();
	}
	return isFinished;
}

void CombinedBonus::Draw()
{
}