#include "stdafx.h"
#include "ChipDistortions.h"
#include "GameSquare.h"

namespace Game
{
	/*
	* ChipDistortion
	*/
	float ChipDistortion::GetAlpha() const
	{
		return 1.f;
	}
	bool ChipDistortion::ShowEye() const
	{
		return false;
	}
	/*
	* ChipJumpOnHang
	*/
	ChipJumpOnHang::ChipJumpOnHang(float pause, float time)
	{
		_timer = - pause;
		_timerScale = 1/time;
	}

	bool ChipJumpOnHang::Update(float dt)
	{
		if(_timer < 0.f)
		{
			_timer += dt;
			if(_timer >= 0.f)
			{
				_timer = 0.f;
			}
		}else{
			_timer += dt*_timerScale;
			if(_timer > 1.f)
			{
				_timer = 1.f;
				return true;
			}
		}
		return false;
	}

	void ChipJumpOnHang::CorrectRect(FRect &rect) const
	{
		if(_timer < 0)
		{
			return;
		}
		float t = math::sin(_timer*math::PI*8.f)*(1 - _timer)*(1 - _timer);
		float dx = rect.Width() * 0.05f * t;
		float dy = rect.Height() * 0.1f * t;
		rect.yEnd -= dy;
		rect.xStart -= dx;
		rect.xEnd += dx;
	}
	/*
	* ChipDistortionFlyAppear
	*/
	ChipDistortionFlyAppear::ChipDistortionFlyAppear(float pause, float time)
	{
		_timer = -pause;
		_timerScale = 1.f/time;
	}

	bool ChipDistortionFlyAppear::Update(float dt)
	{
		if(_timer < 0.f)
		{
			_timer += dt;
			if(_timer >= 0.f)
			{
				_timer = 0.f;
			}
		}else{
			_timer += dt*_timerScale;
			if(_timer > 1)
			{
				return true;
			}
		}
		return false;
	}

	void ChipDistortionFlyAppear::CorrectRect(FRect &rect) const
	{
		//Перенес смещение по y в GameSquare теперь.
		float rt = math::clamp(0.f, 1.f, _timer)/_timerScale;

		float t1 = math::clamp(0.f, 1.f, rt/Game::Square::FLY_SQ_APPEAR_CHIP_ALPHA_PART)*math::clamp(0.f, 1.f, 1 - (_timer-0.9f)/0.1f);
		float h = rect.Height();
		float w = rect.Width();
		rect.yEnd = rect.yStart + h*(1 + t1*0.2f);
		rect.xStart += w*0.1f*t1;
		rect.xEnd   -= w*0.1f*t1;
	}

	float ChipDistortionFlyAppear::GetAlpha() const
	{
		return math::clamp(0.f, 1.f, _timer/_timerScale/Game::Square::FLY_SQ_APPEAR_CHIP_ALPHA_PART);
	}

	bool ChipDistortionFlyAppear::ShowEye() const
	{
		return _timer < 1;
	}

	/*
	* ChipDistortionFlyAppearStop
	*/
	ChipDistortionFlyAppearStop::ChipDistortionFlyAppearStop(float pause, float time)
	{
		_timer = -pause;
		_timerScale = 1.f/time;
	}

	bool ChipDistortionFlyAppearStop::Update(float dt)
	{
		if(_timer < 0.f)
		{
			_timer += dt;
			if(_timer >= 0.f)
			{
				_timer = 0.f;
			}
		}else{
			_timer += dt*_timerScale;
			if(_timer > 1)
			{
				return true;
			}
		}
		return false;
	}

	void ChipDistortionFlyAppearStop::CorrectRect(FRect &rect) const
	{
		float h = rect.Height();
		float w = rect.Width();
		float t = -sinf(_timer*math::PI*4.f)*(1 - _timer);
		rect.yEnd = rect.yStart + h*(1 + t*0.1f);
		rect.xStart += w*0.1f*t;
		rect.xEnd   -= w*0.1f*t;
	}

	float ChipDistortionFlyAppearStop::GetAlpha() const
	{
		return 1.f;
	}

	bool ChipDistortionFlyAppearStop::ShowEye() const
	{
		return false;
	}

	/*
	* ChipDistortionFlyHide
	*/
	ChipDistortionFlyHide::ChipDistortionFlyHide(float pause, float time)
	{
		_timer = -pause;
		_timerScale = 1.f/time;
	}

	bool ChipDistortionFlyHide::Update(float dt)
	{
		if(_timer < 0.f)
		{
			//Пауза
			_timer += dt;
			if(_timer >= 0.f)
			{
				_timer = 0.f;
			}
		}else{
			_timer += dt*_timerScale;
			if(_timer > 1)
			{
				return true;
			}
		}
		return false;
	}

	void ChipDistortionFlyHide::CorrectRect(FRect &rect) const
	{

		float t1 = math::clamp(0.f, 1.f, _timer/0.5f);
		float h = rect.Height();
		float w = rect.Width();
		rect.yEnd = rect.yStart + h*(1 + t1*0.2f);
		rect.xStart += w*0.1f*t1;
		rect.xEnd   -= w*0.1f*t1;

	}

	float ChipDistortionFlyHide::GetAlpha() const
	{
		return 1.f;
	}

	bool ChipDistortionFlyHide::ShowEye() const
	{
		return true;
	}

	/*
	* ChipAppearFromGround
	*/
	ChipAppearFromGround::ChipAppearFromGround(float pause, float time)
	{
		_timer = -pause;
		_timerScale = 1 / time;
	}

	bool ChipAppearFromGround::Update(float dt)
	{
		if (_timer < 0.f)
		{
			_timer += dt;
			if (_timer >= 0.f)
			{
				_timer = 0.f;
			}
		}
		else{
			_timer += dt*_timerScale;
			if (_timer > 1.f)
			{
				_timer = 1.f;
				return true;
			}
		}
		return false;
	}

	void ChipAppearFromGround::CorrectRect(FRect &rect) const
	{
		if (_timer < 0)
		{
			return;
		}
		float t = math::sin(_timer*math::PI*8.f)*(1 - _timer)*(1 - _timer);
		float dx = rect.Width() * 0.1f * t;
		float dy = rect.Height() * 0.4f * t;
		rect.yEnd -= dy;
		rect.xStart -= dx;
		rect.xEnd += dx;
	}

	float ChipAppearFromGround::GetAlpha() const
	{
		return math::clamp(0.f, 1.f, _timer / _timerScale / 0.7f);
	}

}//namespace Game