#ifndef _GAME_FILL_BOUNS_H_
#define _GAME_FILL_BOUNS_H_

#include "GUI/Widget.h"

namespace Match3GUI
{
	class FillBonus
	{
	public:
		static const int COUNTER_DEFAULT = 30;
		static FillBonus& Get();
	public:
		FillBonus();

		void Update(float dt);
		void ChangeCounter(int new_value);
		void SetCounter(int new_value);
		int GetCounter();
		void AddHiddenCounter(int count);
		void ResetHiddenCounter();
		FPoint GetCenterPosition();
	private:
		float _progress;
		int _counter;
		int _hiddenCount; // зачисленные "авансом" ходы, не рисуются, пока не долетит соответствующий эффект
		                  // (и не будет вызван ChangeCounter), но у игрока они как бы уже есть (учитываются в GetCounter)
		bool _active;	
	};
} // namespace Match3GUI

#endif //_GAME_FILL_BOUNS_H_