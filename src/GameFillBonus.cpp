#include "stdafx.h"
#include "GameFillBonus.h"
#include "EditorUtils.h"
#include "GameInfo.h"
#include "Match3Gadgets.h"
#include "GameField.h"
#include "Match3Spirit.h"

namespace Match3GUI
{

	FillBonus::FillBonus()
		: _progress(0.f)
		, _active(false)
		, _counter(COUNTER_DEFAULT)
		, _hiddenCount(0)
	{
	}

	FillBonus& FillBonus::Get()
	{
		static FillBonus instance;
		return instance;
	}


	void FillBonus::Update(float dt)
	{
		if(!_active)
		{
			return;
		}
		int count = _counter;
		if( Gadgets::levelSettings.getString("FILL_ItemType") == "Chips")
			count += gameInfo.getLocalInt("FILL_bonus_current_s_count", 0);
		int count_limit = Gadgets::levelSettings.getInt("FILL_Seq0");
		_progress = math::clamp(0.0f, 1.0f, (float)count/count_limit);

		if(GameField::Get()->IsStandby())
		{
			while(_counter >= count_limit)
			{
				std::string end_bonus_type = Gadgets::levelSettings.getString("FILL_LineBonusType");
				int radius = Gadgets::levelSettings.getInt("FILL_Radius0");
				std::string transform = Gadgets::levelSettings.getString("FILL_ChainChipTransform");
				Game::Hang::TransformChip tr = Game::Hang::NONE;
				if( transform == "chameleon" )
					tr = Game::Hang::CHAMELEON;
				else if( transform == "energy_bonus" )
					tr = Game::Hang::ENERGY_BONUS;
				IPoint dir(1, 0);
				if( math::random(0, 1) > 0 ) dir.x = -1;
				if( math::random(0, 1) > 0 ) std::swap(dir.x, dir.y);
				Game::Hang hang(end_bonus_type, radius, 1, dir, tr, false);

				std::vector<Game::FieldAddress> chips;
				Game::GetAddressesForHangBonus(chips);

				size_t count = Gadgets::levelSettings.getInt("FILL_count_items");
				if( chips.size() < count ) // не можем пока повесить бонус, выходим и ждем удобного момента
					break;

				for(size_t i = 0; i < count; i++)
				{
					FPoint pos = GetCenterPosition();
					AddressVector seq;
					seq.push_back( Game::FieldAddress(0,0) );
					seq.push_back( Game::FieldAddress(0,0) );
					Game::AddController( new Game::Spirit(pos, IPoint(-1, -1), std::string("screen"), hang, seq) );
				}

				_counter -= count_limit;
			}
		}
	}

	int FillBonus::GetCounter()
	{
		return _counter + _hiddenCount;
	}

	void FillBonus::SetCounter(int new_value)
	{
		_active = Gadgets::levelSettings.getBool("FILL_bonus_allow");
		_counter = math::clamp(0, 100000, new_value);
	}

	void FillBonus::ChangeCounter(int delta)
	{
		SetCounter(_counter + delta);
		_hiddenCount -= delta;
	}

	void FillBonus::AddHiddenCounter(int count)
	{
		_hiddenCount += count;
	}

	void FillBonus::ResetHiddenCounter()
	{
		_hiddenCount = 0;
	}


	FPoint FillBonus::GetCenterPosition()
	{
		return FPoint(0,0);
	}
}