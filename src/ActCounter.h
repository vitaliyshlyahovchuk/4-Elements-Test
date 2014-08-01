#pragma once
#include "GUI/Widget.h"

namespace Match3GUI
{
	// Счетчик ходов
	class ActCounter
	{
	private:
		static int _counter;
		static int _hiddenCount; // зачисленные "авансом" ходы, не рисуются, пока не долетит соответствующий эффект
		                         // (и не будет вызван ChangeCounter), но у игрока они как бы уже есть (учитываются в GetCounter)
	public:
		static const int COUNTER_DEFAULT = 30;
		static const int INFINITE_MOVES = 1000;
		static void ChangeCounter(int new_value);
		static void SetCounter(int new_value);
		static int GetCounter();
		static int GetHiddenCount();
		static void AddHiddenCounter(int count);
		static void ResetHiddenCounter();

		//void AcceptMessage(const Message &message);
	};

	class TimeCounter
	{
	private:
		static float _time;
		static float _hiddenTime;
	public:
		static void SetTime(float time);
		static void AddTime(float time);
		static void UpdateTime(float dt);
		static float GetTime();
		static float GetHiddenTime();
		static void AddHiddenTime(float dt);
		static void ResetHiddenTime();

		//void AcceptMessage(const Message &message);
	};
} // namespace Match3GUI