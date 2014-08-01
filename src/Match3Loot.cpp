#include "stdafx.h"
#include "Match3Loot.h"
#include "EditorUtils.h"
#include "GameInfo.h"

namespace Match3GUI
{
	int LootPanel::score = 0;

	void LootPanel::AddScore(int points)
	{
		score += points;
	}

	int LootPanel::GetScore()
	{
		return score;
	}

	void LootPanel::SetScore(int points)
	{
		score = points;
	}
}