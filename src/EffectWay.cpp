#include "stdafx.h"
#include "EffectWay.h"
#include "GameField.h"

namespace Utils
{

	EffectDrawer::EffectDrawer(std::string name, ParticleEffectPtr &eff,  const Color color, bool need_square_scale)
		: GameFieldController("EffectDrawer", 1.f, GameField::Get())
		, _drawType(GDP_UP)
		, _drawTime(0.f)
		, _drawTimeScale(1.f)
		, _alpha(0.f)
		, _color(color)
	{
		if(need_square_scale)
		{
			eff = Game::AddEffect(_effCont, name);
		}else{
			eff = _effCont.AddEffect(name);
		}
	}

	void EffectDrawer::SetDrawType(DrawTypes type, const float time, const float pause)
	{
		_drawType = type;
		_drawTimeScale = 1.f/time;
		_drawTime = -pause;
	}

	void EffectDrawer::Update(float dt)
	{
		_effCont.Update(dt);
		if(_drawType == GDP_SMOOTH)
		{
			if(_drawTime < 0)
			{
				_drawTime += dt;
			}else{
				_drawTime += dt*_drawTimeScale;
			}
			_alpha = math::clamp(0.f, 1.f, _drawTime);
		}
	}

	void EffectDrawer::DrawBase()
	{
		Render::BeginColor(_color);
		_effCont.Draw();
		Render::EndColor();
	}

	void EffectDrawer::Draw()
	{
		if(_drawType == GDP_SMOOTH)
		{
			if(_alpha > 0)
			{
				Render::BeginAlphaMul(_alpha);
				DrawBase();
				Render::EndAlphaMul();
			}
		}else if(_drawType == GDP_UP)
		{
			DrawBase();
		}
	}

	void EffectDrawer::DrawUnderChips()
	{
		if(_drawType == GDP_SMOOTH)
		{
			if(_alpha < 1)
			{
				Render::BeginAlphaMul(1.f - _alpha);
				DrawBase();
				Render::EndAlphaMul();
			}
		}else if(_drawType == GDP_DOWN)
		{
			DrawBase();
		}	
	}

	bool EffectDrawer::isFinish()
	{
		return _effCont.IsFinished();
	}

} //namespace Utils