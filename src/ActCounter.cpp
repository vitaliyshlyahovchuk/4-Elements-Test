#include "stdafx.h"
#include "ActCounter.h"
#include "GameInfo.h"

namespace Match3GUI
{
	int ActCounter::_counter(COUNTER_DEFAULT);
	int ActCounter::_hiddenCount(0);

	int ActCounter::GetCounter()
	{
		return _counter + _hiddenCount;
	}

	int ActCounter::GetHiddenCount()
	{
		return _hiddenCount;
	}

	void ActCounter::SetCounter(int new_value)
	{
		_counter = new_value;
	}

	void ActCounter::ChangeCounter(int delta)
	{
		if(_counter < INFINITE_MOVES )
		{
			ActCounter::SetCounter(_counter + delta);
			if(delta > 0 && _hiddenCount >= delta)
				_hiddenCount -= delta;
		}
	}

	void ActCounter::AddHiddenCounter(int count)
	{
		if(_counter < INFINITE_MOVES)
			_hiddenCount += count;
	}

	void ActCounter::ResetHiddenCounter()
	{
		_hiddenCount = 0;
	}

	/*void ActCounter::AcceptMessage(const Message &message)
	{
		if(message.is("SetCounter"))
		{
			ActCounter::SetCounter(message.getIntegerParam());
		}
		else if(message.is("ChangeCounter"))
		{
			ActCounter::ChangeCounter(message.getIntegerParam());
		}
	}*/


	float TimeCounter::_time = 0.0f;
	float TimeCounter::_hiddenTime = 0.0f;

	void TimeCounter::SetTime(float time)
	{
		_time = time;
	}

	void TimeCounter::AddTime(float dtime)
	{
		_time += dtime;
		if(dtime > 0 && _hiddenTime >= dtime)
			_hiddenTime -= dtime;
	}

	float TimeCounter::GetTime()
	{
		return _time + _hiddenTime;
	}

	float TimeCounter::GetHiddenTime()
	{
		return _hiddenTime;
	}

	void TimeCounter::UpdateTime(float dt)
	{
		_time -= dt;
	}

	void TimeCounter::AddHiddenTime(float time)
	{
		_hiddenTime += time;
	}

	void TimeCounter::ResetHiddenTime()
	{
		_hiddenTime = 0.0f;
	}

	/*void TimeCounter::AcceptMessage(const Message &message)
	{
		if(message.is("SetCounter"))
		{
			TimeCounter::SetTime((float)message.getIntegerParam());
		}
		else if(message.is("ChangeCounter"))
		{
			TimeCounter::AddTime((float)message.getIntegerParam());
		}
	}*/

} // Match3GUI namespace