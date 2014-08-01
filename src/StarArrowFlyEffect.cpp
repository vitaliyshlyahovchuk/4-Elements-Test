#include "stdafx.h"
#include "StarArrowFlyEffect.h"
#include "GameField.h"

namespace Match3
{
	const FPoint STAR_JUMP_POS = FPoint(0.f, 40.f);
	//const FPoint STAR_OFFSET_TO_CENTER = FPoint(-41.f, -41.f);

	FlyStarArrowEffect::FlyStarArrowEffect(const FPoint pos, const FPoint offset_in_cell, Render::Texture *texture, const float angle, float pause)
		: GameFieldController("FlyStarArrowEffect", 1.f, GameField::Get())
		, _time(-pause)
		, _localTime(0.f)
		, _state(FLYSTAR_PAUSE)
		, _pause(pause)
		, _flyEffect(NULL)
		, _messageWidget(NULL)
		, _endAngle(angle)
		, _offset_in_cell(offset_in_cell)
	{
		_texture = texture;
		_centerTexture = FPoint(_texture->getBitmapRect().Width()/2.f, _texture->getBitmapRect().Height()/2.f);

		_posStartOnField = pos;
		_centerPoint = _posStartOnField + FPoint(0.f, 50.f);
		_v0 = GameSettings::ToScreenPos(_centerPoint); //Точка старта

		FPoint v1 = Core::LuaCallFunction<FPoint>("gameInterfaceCounterPosition") - _centerTexture - _offset_in_cell; //Точка на панели

		_spline.Clear();
		_spline.addKey(_v0);
		FPoint dir = (v1 -_v0).Rotated(math::PI*0.5f*(math::random(100)<50?1.f:-1.f));
		_spline.addKey(_v0*0.4f + v1*0.6f + dir.Normalized()*100.f);
		_spline.addKey(v1);
		_spline.CalculateGradient();

		MM::manager.PlaySample("AddMovesActivate");
	}

	FlyStarArrowEffect::~FlyStarArrowEffect()
	{
		 int iii =0;
	}

	void FlyStarArrowEffect::Update(float dt)
	{
		_localTime += dt;
		if(_state == FLYSTAR_PAUSE)
		{
			if(_time <= 0)
			{
				_time += dt;
				if(_time > 0)
				{
					_time = 0.f;
					_state = FLYSTAR_JUMP_TO_CENTER;
					_flyEffect = Game::AddEffect(_effCont, "StarActFly");
					_flyEffect->Reset();
					Assert(_flyEffect->IsPermanent());
				}
			}
		}else if(_state == FLYSTAR_JUMP_TO_CENTER)
		{
			if(_time < 1)
			{
				_time += dt*0.6f;
				if(_time > 1)
				{
					_time = 0.f;
					_state = FLYSTAR_FLY;
				}
			}
		}else if(_state == FLYSTAR_FLY)
		{
			if(_time < 1)
			{
				_time += dt*1.0f;
				if(_time > 1)				{
					_time = 0.f;
					_state = FLYSTAR_HIDE;
					ParticleEffect* eff = _effContInner.AddEffect("StarActFinish");
					eff->SetPos(0.f, 0.f);
					eff->Reset();
					_flyEffect->Finish();
					_flyEffect.reset();
				}
			}
		}else if(_state == FLYSTAR_HIDE)
		{
			if(_time < 1)
			{
				_time += dt*5.0f;
				if(_time > 1)
				{
					_time = 0.f;
					_state = FLYSTAR_WAIT;
					if(_messageWidget)
					{
						_messageWidget->AcceptMessage(_messageEnd);
					}
				}
			}
		}else if(_state == FLYSTAR_WAIT)
		{
			if(_time < 1)
			{
				_time += dt*1.0f;
				if(_time > 1)
				{
					_time = 0.f;
					_state = FLYSTAR_FINISH;
				}
			}
		}
		_effCont.Update(dt);
		_effContInner.Update(dt);
	}

	void FlyStarArrowEffect::DrawAbsolute()
	{
		float alpha = 1.f;
		FPoint pos;
		float scale_x = 1.0f;
		float scale_y = 1.0f;
		float angle = _endAngle;
		if(_state == FLYSTAR_PAUSE)
		{
			pos = GameSettings::ToScreenPos(_posStartOnField); //Сначала стрела привязана к полю!
			//alpha = 1.f;
			scale_x  = scale_y = 1.f;
			angle = 0.f;
		}
		else if(_state == FLYSTAR_JUMP_TO_CENTER)
		{
			//В течении прыжка мы плавно перейдем из координат поля в координаты экрана
			pos = math::lerp(GameSettings::ToScreenPos(_posStartOnField), GameSettings::ToScreenPos(_centerPoint), math::ease(_time, 0.5f, 0.2f));
			pos.y += 10.f*sinf(_time*math::PI);

			angle = math::lerp(0.f, _endAngle, _time);

			float t0 = math::clamp(0.f, 1.f, (_time - 0.2f)/0.8f);
			float scale_dt = 0.1f*sinf(t0*math::PI*2.f*2.f);
			scale_x = 1.f + scale_dt;
			scale_y = 1.f - scale_dt;
			float scale_full = 0.2f*sinf(t0*math::PI);
			scale_x += scale_full;
			scale_y += scale_full;
		}
		else if(_state == FLYSTAR_FLY)
		{
			pos = math::lerp(GameSettings::ToScreenPos(_centerPoint), _spline.getGlobalFrame(_time), _time);
			scale_x = scale_y = math::clamp(0.f, 1.f, cosf(_time*math::PI*0.5f) + 0.4f*_time);
		}
		else if(_state == FLYSTAR_HIDE)
		{
			pos = _spline.getGlobalFrame(1.f);
			scale_x = scale_y = 0.4f;
			alpha = math::clamp(0.f, 1.f, cosf(_time*math::PI*0.5f));
		}
		else if(_state == FLYSTAR_WAIT)
		{
			pos = _spline.getGlobalFrame(1.f);
			scale_x = scale_y = 0.4f;
			alpha = 0.f;
		}
		else if(_state == FLYSTAR_FINISH)
		{
			pos = _spline.getGlobalFrame(1.f);
			alpha =0.f;
			scale_x = scale_y = 0.4f;
		}
		if(_flyEffect.get())
		{
			_flyEffect->SetPos(pos + _offset_in_cell + _centerTexture);
		}
		_effCont.Draw();


		Render::device.PushMatrix();
		Render::device.MatrixTranslate(pos);
		Game::MatrixSquareScale();
		Render::device.MatrixTranslate(_offset_in_cell);
		Render::device.MatrixTranslate(_centerTexture);
		Render::device.MatrixScale(scale_x, scale_y, 1.f);	

		_effContInner.Draw();

		Render::BeginAlphaMul(alpha);
		Render::device.MatrixRotate(math::Vector3(0.f, 0.f, 1.f), angle);
		_texture->Draw(-_centerTexture);
		Render::EndAlphaMul();

		Render::device.PopMatrix();

	}

	bool FlyStarArrowEffect::isFinish()
	{
		return _state == FLYSTAR_FINISH;
	}

	void FlyStarArrowEffect::AddMessage(const Message &message, GUI::Widget *widget)
	{
		if(!widget)
		{
			MyAssert(false);
			return;
		}
		_messageEnd = message;
		_messageWidget = widget;
	}
}//namespace Match3