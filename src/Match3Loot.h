#ifndef MATCH3_LOOT_H
#define MATCH3_LOOT_H

#include "GUI/Widget.h"

namespace Match3GUI
{
	class LootPanel
	{
		static int score;
	public:
		static void AddScore(int points);
		static int GetScore();
		static void SetScore(int points);
	};
} // namespace Match3GUI

#endif //MATCH3_LOOT_H