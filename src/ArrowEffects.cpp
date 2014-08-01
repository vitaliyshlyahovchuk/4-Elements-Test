#include "stdafx.h"
#include "ArrowEffects.h"
//#include "Game.h"
#include "GameField.h"
#include "GameInfo.h"

namespace Game
{
	Render::Texture *arrowBonusStrip = NULL;
	Render::ShaderProgram *arrowShader = 0;
	float ARROW_BONUS_SPEED = 17.0f;
	float ARROW_BONUS_STRIP_MAX_LENGHT = 3.f;
	float ARROW_BONUS_EFFECT_START_OFFSET[8] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
	float ARROW_BONUS_2_EFFECT_DELAY = 0.f;
	float ARROW_BONUS_EFFECT_HEAD_OFFSET = 0.f;

	void InitArrowEffects(rapidxml::xml_node<> *xml_info)
	{
		arrowBonusStrip = Core::resourceManager.Get<Render::Texture>("HangArrowStrip");	
		arrowBonusStrip->setAddressType(Render::Texture::REPEAT);
		arrowShader = Core::resourceManager.Get<Render::ShaderProgram>("ArrowStrip");
		ARROW_BONUS_SPEED = gameInfo.getConstFloat("ARROW_BONUS_SPEED", 17.f);
		ARROW_BONUS_STRIP_MAX_LENGHT = gameInfo.getConstFloat("ARROW_BONUS_STRIP_MAX_LENGHT", 3.f)*GameSettings::SQUARE_SIDEF;
		for(size_t i = 0; i < 8; i++)
		{
			ARROW_BONUS_EFFECT_START_OFFSET[i] = gameInfo.getConstFloat("ARROW_BONUS_EFFECT_START_OFFSET_" + utils::lexical_cast(i), 0.f)*GameSettings::SQUARE_SIDEF;
		}
		ARROW_BONUS_2_EFFECT_DELAY = gameInfo.getConstFloat("ARROW_BONUS_2_EFFECT_DELAY", 0.f);
		ARROW_BONUS_EFFECT_HEAD_OFFSET = gameInfo.getConstFloat("ARROW_BONUS_EFFECT_HEAD_OFFSET", 0.f)*GameSettings::SQUARE_SIDEF;		
	}


	ArrowBonusController::Arrow::Arrow(IPoint start, IPoint direction, int rad, float time_scale_, Byte dir_id)
		: dir(direction)
		, chips(1)
		, next_lenght(0.f)
		, _effFront(0)
		, _effBack(0)
		, local_time(0.f)
		, time_scale(time_scale_)
		, _lenght(0.f)
		, _stripTime(0.f)
		, _dir_type(dir_id)
		, _alphaFrontEffect(1.f)
		, _effFrontIsRan(false)
	{
		start_pos = FPoint(start) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;

		radius = (rad + 0.5f) * GameSettings::SQUARE_SIDEF;
	}

	ArrowBonusController::Arrow::~Arrow()
	{
		OnFinish();
	}

	void ArrowBonusController::Arrow::OnFinish()
	{
		if(_effFront)
		{
			//ѕервый эффект можно удалить , он ушел в альфу полностью.
			_effFront->Kill();
			_effFront.reset();
		}	
		if(_effBack)
		{
			_effBack->Finish();
			_effBack.reset();
		}
	}

	bool ArrowBonusController::Arrow::Update(float dt)
	{
		_stripTime += dt*6.f;
		local_time += time_scale * dt;	
		_lenght = math::clamp(0.f, radius, local_time * ARROW_BONUS_SPEED * GameSettings::SQUARE_SIDEF);
		math::Vector3 pos = start_pos + dir *_lenght;
		math::Vector3 pos_effect = pos + dir*ARROW_BONUS_EFFECT_HEAD_OFFSET;
		if(_lenght > ARROW_BONUS_EFFECT_START_OFFSET[_dir_type] && !_effFrontIsRan)
		{
			_effFrontIsRan = true;
			//Ёффекта еще нет - создаем
			std::string name_effect = "";
			std::string name_effect_back = "";
			if(dir.x == 1 && dir.y == 0)
			{
				name_effect = "Hang_front_0";
				name_effect_back = "Hang_back_0";
			}
			else if(dir.x == 0 && dir.y == 1)
			{
				name_effect = "Hang_front_1";
				name_effect_back = "Hang_back_1";
			}
			else if(dir.x == -1 && dir.y == 0)
			{
				name_effect = "Hang_front_2";
				name_effect_back = "Hang_back_2";
			}
			else if(dir.x == 0 && dir.y == -1)
			{
				name_effect = "Hang_front_3";
				name_effect_back = "Hang_back_3";
			}
			if(!name_effect.empty())
			{
				_effFront = Game::AddEffect(GameField::Get()->_effTopCont, name_effect);
				_effFront->SetPos(pos_effect.x, pos_effect.y);
				_effFront->Reset();

				_effBack = Game::AddEffect(GameField::Get()->_effTopCont, name_effect_back);
				_effBack->SetPos(pos_effect.x, pos_effect.y);
				_effBack->Reset();
			}	
		}
		if(_effFront.get())
		{
			_effFront->SetPos(pos_effect.x, pos_effect.y);
			if(radius - _lenght < GameSettings::SQUARE_SIDEF)
			{
				_alphaFrontEffect = math::lerp(0.f, 1.f, (radius - _lenght)/(GameSettings::SQUARE_SIDEF*0.5f));
				_effFront->SetAlphaFactor(_alphaFrontEffect);
			}
		}
		if(_effBack.get())
		{
			_effBack->SetPos(pos_effect.x, pos_effect.y);
		}
		if(radius <= _lenght)
		{
			OnFinish();
			return true;
		}
		return false;
	}

	Render::QuadVertT2 CreateQuadVertT2(math::Vector3 v, Color &c, float u1, float v1, float u2, float v2)
	{
		return Render::QuadVertT2(v.x, v.y, v.z, c, u1, v1, u2, v2);
	}

	void ArrowBonusController::Arrow::Draw()
	{
		//float thick = math::lerp(0.f, 0.5f,_lenght/GameSettings::SQUARE_SIDEF)*GameSettings::SQUARE_SIDEF;

		math::Vector3 n(dir.y, -dir.x, 0.f);
		n.Normalize();
		n *= GameSettings::SQUARE_SIDEF*0.5f;
		math::Vector3 dir_absolute = dir * _lenght; //јбсолютный вектор стрелы
		math::Vector3 start(0.f, 0.f, 0.f);
		if(dir_absolute.Length() > ARROW_BONUS_STRIP_MAX_LENGHT)
		{
			start = dir*(dir_absolute.Length() - ARROW_BONUS_STRIP_MAX_LENGHT);
			dir_absolute.Normalize();
			dir_absolute = dir_absolute*ARROW_BONUS_STRIP_MAX_LENGHT;
		}

		float start_u_mask = math::clamp(0.f, 1.f, 1.f - dir_absolute.Length()/ARROW_BONUS_STRIP_MAX_LENGHT);
		Color c(255, 255, 255, math::lerp(0, 255, _alphaFrontEffect));

		Render::QuadVertT2 quads[4];

		math::Vector3 p1 = start_pos + start - n;
		math::Vector3 p3 = start_pos + start + n;
	
		quads[0] =  CreateQuadVertT2(p1, c, start_u_mask, 0.f, _stripTime, 0.f);
		quads[1] =  CreateQuadVertT2(p1 + dir_absolute, c, 1.f, 0.f, _stripTime + 1.f, 0.f);
		quads[2] =  CreateQuadVertT2(p3, c, start_u_mask, 1.f, _stripTime, 1.f);
		quads[3] =  CreateQuadVertT2(p3 + dir_absolute, c, 1.f, 1.f, _stripTime + 1.f, 1.f);

		Render::device.DirectDrawQuad(quads);
	}

	ArrowBonusController::ArrowBonusController(float startTime, FPoint start_pos, int type)
		: GameFieldController("ArrowBonus", 1.0f, GameField::Get())
		, is_finished(false)
		, _effectName("")
		, _start_pos(start_pos)
		, _effect2Timer(ARROW_BONUS_2_EFFECT_DELAY)
		, _effect2Runing(false)
	{
		z = 3;
		local_time = -startTime;
		if(type == 1){
			_effectName = "Hang_hor";
		}else if(type == 2){
			_effectName = "Hang_vert";
		}else{
			_effectName = "Hang_cross";
		}
		ParticleEffectPtr eff = Game::AddEffect(gameField->_effTopCont, _effectName);
		eff->SetPos(start_pos);
		eff->Reset();
	}

	void ArrowBonusController::AddArrow(IPoint start, IPoint dir, int radius, Byte dir_id)
	{
		float moveTime = radius/(ARROW_BONUS_SPEED * GameSettings::SQUARE_SIDEF);

		Arrow arr(start, dir, radius, time_scale, dir_id);

		_arrows.push_back( arr );

	}

	void ArrowBonusController::Update(float dt)
	{
		local_time += dt;
		_effect2Timer -= dt;
		if( local_time < 0.0f ) {
			return;
		}
		is_finished = true;
		for(size_t i = 0; i < _arrows.size(); ++i)
		{
			Game::ArrowBonusController::Arrow &arrow = _arrows[i];
			if(arrow.Update(dt))
			{
			}else{
				is_finished = false;
			}
		}
		if(_effect2Timer < 0 && !_effect2Runing)
		{
			_effect2Runing = true;
			ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effTopCont, _effectName + "_2");
			eff->SetPos(_start_pos);
			eff->Reset();
		}
	}

	void ArrowBonusController::Draw()
	{
		if( local_time < 0.0f )
		{
			return;
		}

		if(!_arrows.empty())
		{
			arrowShader->Bind();
			arrowBonusStrip->Bind();
			for(Arrow &arr : _arrows){
				arr.Draw();
			}
			arrowShader->Unbind();
		}
	}

	bool ArrowBonusController::isFinish()
	{
		return is_finished;
	}
}
