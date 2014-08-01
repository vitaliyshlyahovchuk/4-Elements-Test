#include "stdafx.h"
#include "ReceiverEffects.h"
#include "GameField.h"
#include "GameInfo.h"
#include "EnergyReceivers.h"

namespace Receiver
{
	FPoint JUMP_POS = FPoint(0.f, 120.f);
	const FPoint CRYSTAL_POS_ANIM_ON_SQUARE = FPoint(123.f, 90.f);
	const FPoint CRYSTAL_SCALE_CENTER = FPoint(123.f, 138.f);

	bool ReceiverEffect::keep_camera = false;

	ReceiverEffect::ReceiverEffect(const FPoint pos, std::string uid, int current_frame)
		: GameFieldController("ReceiverEffect", 1.f, GameField::Get())
		, _time(0.f)
		, _localTime(0.f)
		, _state(RE_JUMP)
		, _uid(uid)
		, _finishEffectRunning(false)
	{
		JUMP_POS.y = gameInfo.getConstFloat("RECEIVER_JUMP_HEIGHT", 120.f);
		_jumpTmeScale = 1.f/gameInfo.getConstFloat("RECEIVER_JUMP_TIME", 0.5f);
		_stayTimeScale = 1.f/gameInfo.getConstFloat("RECEIVER_STAY_TIME", 0.5f);
		_flyTimeScale = 1.f/gameInfo.getConstFloat("RECEIVER_FLY_TIME", 1.f);
		_jumpScale = gameInfo.getConstFloat("RECEIVER_JUMP_SCALE", 2.f);
		
		_crystalAnim = Render::StreamingAnimation::Spawn("CrystalRotate");
		_crystalAnim->SetCurrentFrame(current_frame);
		_frame1 = current_frame;
		_frame2 = (_crystalAnim->GetLastPlayedFrame() - _crystalAnim->GetFirstPlayedFrame())*gameInfo.getConstInt("RECEIVER_ROTATE_COUNT", 3) + gameInfo.getConstInt("RECEIVER_LAST_FRAME", 3);
		
		_crystalTime = 0.f;
		_crystalTimeScale = _jumpTmeScale*_stayTimeScale/(_jumpTmeScale + _stayTimeScale);

		_posStartOnField = pos + FPoint(GameSettings::CELL_HALF.x, 0.f);
		_v0 = GameSettings::ToScreenPos(_posStartOnField + JUMP_POS); //Точка старта
		_lootPos = Core::LuaCallFunction<FPoint>("gameInterfaceReceiverPosition");
		FPoint v1 = _lootPos + CRYSTAL_POS_ANIM_ON_SQUARE - CRYSTAL_SCALE_CENTER; //Точка на панели
		_finishScaleCrystal = FPoint(0.75f, 0.75f);

		_spline.Clear();
		_spline.addKey(_v0);
		//FPoint dir = (v1 -_v0).Rotated(math::PI * 0.5f * (math::random(0,1) ? 1.f : -1.f));
		//FPoint dir = (v1 -_v0).Rotated(math::PI * 0.5f * -1.f);
		//_spline.addKey(_v0*0.5f + v1*0.5f + dir.Normalized()*100.f);
		_spline.addKey(v1);
		_spline.CalculateGradient();

		_angleSpline.Clear();
		float angle_prev = _spline.getGlobalFrame(0.f).GetDirectedAngleNormalize( FPoint(0.f, 1.f))*180.f/math::PI;
		if(angle_prev > 180.f)
		{
			angle_prev -= 360.f;
		}
		for(size_t i = 0; i <=2; i++)
		{
			float t = float(i)/2.f;
			float angle = _spline.getGlobalFrame(t).GetDirectedAngleNormalize(FPoint(0.f, 1.f))*180.f/math::PI;
			if(angle > 180.f)
			{
				angle -= 360.f;
			}
			while(abs(angle - angle_prev) > 180.f)
			{
				if(angle > angle_prev)
				{
					angle -= 360.f;
				}else{
					angle += 360.f;
				}
			}
			_angleSpline.addKey(angle);		
			angle_prev = angle;
		}
		_angleSpline.CalculateGradient();

		gameField->AddEnergyWave(pos, "EnergyWaveBig");

		//_idSample = MM::manager.PlaySample("RecieverFly",true);

        ParticleEffectPtr eff = Game::AddEffect(_effCont, "RecBreakFront");
		eff->SetPos(GameSettings::ToScreenPos(_posStartOnField + JUMP_POS));
		eff->Reset();
		ReceiverEffect::keep_camera = true;
	}

	ReceiverEffect::~ReceiverEffect()
	{
	}

	void ReceiverEffect::Update(float dt)
	{
		_localTime += dt;
		if(_crystalTime < 1)
		{
			_crystalTime = math::clamp(0.f, 1.f, _crystalTime + dt*_crystalTimeScale);
			int frames_count = _crystalAnim->GetLastPlayedFrame() - _crystalAnim->GetFirstPlayedFrame();
			int frame = _crystalAnim->GetFirstPlayedFrame() + (math::lerp(_frame1, _frame2, _crystalTime) % frames_count);
			_crystalAnim->SetCurrentFrame(frame);		
		}
		if(_state == RE_JUMP)
		{
			ReceiverEffect::keep_camera = true;
			if(_crystalAnim)
			{
				//_crystalAnim->Update(dt*math::lerp(0.f, _crystalTimeScale*2.f, sinf(math::ease(_time, 0.1f, 0.9f)*math::PI)));
			}
			if(_time < 1)
			{
				_time += dt*_jumpTmeScale;
				if(_time > 1)
				{
					_time = 0.f;
					_state = RE_STAY;		

					_flyEffect = _effCont.AddEffect("RecFly");
					_flyEffect->SetPos(GameSettings::ToScreenPos(_posStartOnField));
					_flyEffect->Reset();
					Assert(_flyEffect->IsPermanent());

				}
			}
		}
		else if(_state == RE_STAY)
		{
			ReceiverEffect::keep_camera = true;
			if(_time < 1)
			{
				_time += dt*_stayTimeScale;
				if(_time > 1)
				{
					_state = RE_FLY;
					_time = 0.f;
				}
			}
		}
		else if(_state == RE_FLY)
		{
			if(_time < 1)
			{
				_time += dt*_flyTimeScale;
				if(!_finishEffectRunning && (1 - _time)/_flyTimeScale < 0.5f)
				{
					//За 0.5с до прилета кристалла в лут запускаем эффект прилета
					_finishEffectRunning = true;
					ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effContUp, "RecFinish");
					eff->SetPos(_lootPos);
					eff->Reset();
				}
				if(_time > 1)
				{
					_time = 0.f;
					_state = RE_HIDE;
					if(_flyEffect)
					{
						_flyEffect->Finish();
						_flyEffect.reset();
					}
					Core::LuaCallVoidFunction("gameInterfaceRunReceiverChangeEffect");
					//заменяем звук
					//MM::manager.StopSample(_idSample);
					//_idSample = MM::manager.PlaySample("RecieverLanded");
				}
			}
		}else if(_state == RE_HIDE)
		{
			if(_time < 1)
			{
				_time += dt*0.5f;
				if(_time > 1)
				{
					_time = 0.f;
					_state = RE_FINISH;
					Gadgets::receivers.AcceptMessage(Message("DestroyFinished", _uid));
				}
			}
		}
		//else if(_state == RE_WAIT)
		//{
		//	if(_time < 1)
		//	{
		//		_time += dt*1.0f;
		//		if(_time > 1)
		//		{
		//			_time = 0.f;
		//			_state = RE_FINISH;

		//		}
		//	}
		//}
		_effCont.Update(dt);
		_effContInner.Update(dt);
	}

	void ReceiverEffect::DrawAbsolute()
	{
		float alpha = 1.f;
		FPoint pos;
		FPoint scale(1.0f, 1.f);
        float angle = 0.0f;
        
		if(_state == RE_JUMP)
		{
			float t = _time;
			pos = GameSettings::ToScreenPos(_posStartOnField);
			pos += math::lerp(FPoint(0.f, 0.f), JUMP_POS, math::ease(t, 0.9f, 0.1f));
            
			scale = math::lerp(FPoint(1.f, 1.f), FPoint(_jumpScale, _jumpScale), t);
		}
		else if(_state == RE_STAY)
		{
			scale = FPoint(_jumpScale, _jumpScale);
			pos = GameSettings::ToScreenPos(_posStartOnField + JUMP_POS);
		}
		else if(_state == RE_FLY)
		{
			float t = math::ease(math::clamp(0.f, 1.f, (_time-0.2f)/0.8f), 0.4f, 0.1f);
			//float t = math::clamp(0.f, 1.f, (_time-0.2f)/0.8f);
            
            
			pos = _spline.getGlobalFrame(t);

			//pos.y += 150.f*math::clamp(0.f, 1.f, _time/0.5f)*math::clamp(0.f, 1.f, 1 - (_time - 0.4f)/0.6f);
            
			scale = math::lerp(FPoint(_jumpScale, _jumpScale), _finishScaleCrystal, t);

			scale.y *= 1 - 0.3f*sinf(math::clamp(0.f, 1.f, _time/0.4f)*math::PI*1.5f);
			
			//angle = _angleSpline.getGlobalFrame(t);
			//angle = math::lerp(0.f, angle, t/0.3f);
			//angle = math::lerp(angle, 0.f, (t - 0.7f)/0.3f);
			FPoint dir = _spline.getGlobalFrame(1.f) - pos;
			if(dir.Length() > 0)
			{
				angle = (dir.GetAngle()*180.f/math::PI - 90.f)*math::clamp(0.f, 1.f, (_time- 0.f)/0.2f);
				_lastAngle = angle;
			}	
			alpha = math::clamp(0.f, 1.f, 1 - (t - 0.95f)/0.05f);
		}
		else if(_state == RE_HIDE)
		{
			pos = _spline.getGlobalFrame(1.f);
			scale = _finishScaleCrystal;
			scale.y *= 1.3f;
			angle = _lastAngle;
			alpha = 0.f; // math::clamp(0.f, 1.f, math::cos(math::clamp(0.f, 1.f, _time*4.f)*math::PI*0.5f));
		}
		//else if(_state == RE_WAIT)
		//{
		//	pos = _spline.getGlobalFrame(1.f);
		//	alpha = math::clamp(0.f, 1.f, math::cos(_time*math::PI*0.5f));
		//	scale = _finishScaleCrystal;
		//	angle = _lastAngle;
		//}
		else if(_state == RE_FINISH)
		{
			pos = _spline.getGlobalFrame(1.f);
			alpha = 0.f;
			scale = _finishScaleCrystal;
			scale.y *= 1.3f;
			angle = _lastAngle;
		}
		if(_flyEffect)
		{
			_flyEffect->SetPos(pos);
		}
		_effCont.Draw();


		Render::device.PushMatrix();
		Render::device.MatrixTranslate(pos);

		Render::device.MatrixTranslate(-CRYSTAL_POS_ANIM_ON_SQUARE + CRYSTAL_SCALE_CENTER);
        Render::device.MatrixRotate(math::Vector3::UnitZ, angle);
		Render::device.MatrixTranslate(CRYSTAL_POS_ANIM_ON_SQUARE - CRYSTAL_SCALE_CENTER);

		Render::device.PushMatrix();
		Render::device.MatrixScale(scale.x, scale.y, 1.0f);	

		_effContInner.Draw();

		Render::BeginAlphaMul(alpha);

		Render::device.MatrixTranslate(-CRYSTAL_POS_ANIM_ON_SQUARE);
		_crystalAnim->Draw();

		Render::EndAlphaMul();

		Render::device.PopMatrix();
		Render::device.PopMatrix();
	}

	bool ReceiverEffect::isFinish()
	{
		return _state == RE_FINISH;
	}
}//namespace Receiver