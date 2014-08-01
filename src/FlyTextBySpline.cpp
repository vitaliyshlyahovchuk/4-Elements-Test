#include "stdafx.h"
#include "FlyTextBySpline.h"

namespace Match3GUI
{
	FlyTextBySpline::FlyTextBySpline(const std::string  &text, const FPoint &pos_from, const FPoint &pos_to, float time_scale, GameField *gamefield_, const bool &absolute, Render::Texture *back_texture)
		:GameFieldController("FlyTextBySpline", time_scale, gamefield_)
		, _timer(0.f)
		, _text(text)
		, _time_scale(time_scale)
		, _absolute(absolute)
		, _messageWidget(NULL)
		, _backTexture(back_texture)
		, _va(BottomAlign)
		, _ha(LeftAlign)
	{
		_spline.Clear();
		_spline.addKey(pos_from);
		_spline.addKey(math::lerp(pos_from, pos_to, 0.5f) + FPoint(0.f, math::random(-30.f, 30.f)));
		_spline.addKey(pos_to);
		_spline.CalculateGradient();
		_pos = _spline.getGlobalFrame(0.f);


		if(_backTexture)
		{
			_backCenter = FPoint(_backTexture->getBitmapRect().RightTop()) * 0.5f;
			_va = _ha = CenterAlign;
		}
	}

	void FlyTextBySpline::DrawAbsolute()
	{
		if(_absolute)
		{
			if(_backTexture)
			{
				_backTexture->Draw(_pos - _backCenter);
			}
			Render::FreeType::BindFont("Score");
			//float t = sinf(_timer*math::PI/2.f);
			//Render::BeginAlphaMul(math::clamp(0.f, 1.f, sinf(_timer*math::PI)));
			Render::PrintString(_pos, _text, 1.f, _ha, _va);
			//Render::EndAlphaMul();
		}
	}

	void FlyTextBySpline::Draw()
	{
		if(!_absolute)
		{
			if(_backTexture)
			{
				_backTexture->Draw(_pos - _backCenter);
			}
			Render::FreeType::BindFont("Score");
			//float t = sinf(_timer*math::PI/2.f);
			//Render::BeginAlphaMul(math::clamp(0.f, 1.f, sinf(_timer*math::PI)));
			Render::PrintString(_pos, _text, CenterAlign, TopAlign);
			//Render::EndAlphaMul();
		}
	}

	void FlyTextBySpline::Update(float dt)
	{
		if(_timer < 0)
		{
			_timer += dt;
			if(_timer > 0)
			{
				_timer = 0.f;
			}
		}else{
			_timer += dt*_time_scale;
			if(_timer >= 1.f)
			{
				_timer = 1.f;
			}
			_pos = _spline.getGlobalFrame(math::ease(_timer, 0.3f, 0.3f));
		}
		//GameFieldController::Update(dt);
	}

	bool FlyTextBySpline::isFinish()
	{
		bool finish = _timer >= 1.f;
		if(finish)
		{
			if(_messageWidget)
			{
				_messageWidget->AcceptMessage(_messageEnd);
			}
			_messageWidget = NULL;
			return true;
		}
		return false;
	}

	void FlyTextBySpline::AddMessage(const Message &message, GUI::Widget *widget)
	{
		if(!widget)
		{
			MyAssert(false);
			return;
		}
		_messageEnd = message;
		_messageWidget = widget;
	}

} //Match3GUI

