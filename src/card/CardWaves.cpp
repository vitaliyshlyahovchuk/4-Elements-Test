		#include"stdafx.h"
#include "CardWaves.h"


namespace Card
{
	LightWaves lightWaves;

	/*****************************/
	// LightWave
	/*****************************/

	LightWave::LightWave()
		: timer(0.f)
		, time_scale(1.f)
		, fill(false)
		, width(10.f)
		, D(200.f*2.f)
		, add(false)
		, _alpha(0.01f)
		, globalAlpha(1.f)
		, finished(false)
		, scaling(true)
	{
	}

	LightWave::~LightWave()
	{
	
	}

	void LightWave::Draw(Render::ShaderProgram *shader)
	{
		float pscale = width/D;
		float scale = 0.25f;
		if(scaling)
		{
			if(time > 0)
			{
				scale = pscale/timer;
				if(scale > 0.25f)
				{
					scale = 0.25f;
				}
			}
		}else{
			scale = 0.25f*width;
		}
		float params[4] = {
							scale, //“екущий масштаб ширины волны. Ќе посто€нно, т.к. чем больше квдрат тем тоньше нужно рисовать волну
							_alpha*globalAlpha, //√лобальна€ альфа волны
							fill ? 1.f: 0.f, //0 - не заполн€ть центр волны, 1 - залить центр
							add ? 1.f: 0.f //ADD front
							};

		shader->setPSParam("params", params, 4);

		//ЦEOUAI ?ЗO?E????EEO? IВЗЙ?ЗU
		FRect rect = FRect(-0.5f, 0.5f, -0.5f, 0.5f);
		rect.Inflate(timer*D);
		rect.MoveBy(pos);
		Render::DrawRect(rect, FRect(0.f, 1.f, 0.f, 1.f));
	}
	
	bool LightWave::Update(float dt)
	{
		if(timer < 0)
		{
			timer += dt;
			if(timer >= 0.f)
			{
				timer = 0.f;
			}
			_alpha = 0.f;
			return false;
		}
		timer += dt*time_scale;
		_alpha = (1.f - math::clamp(0.0f, 1.0f, (timer - 0.7f)/0.3f));
		finished = timer >= 1.f;
		return finished;
	}

	void LightWave::SetRadius(const float r)
	{
		D = r*2;
	}

	/*****************************/
	// LightWavePuls
	/*****************************/

	LightWavePuls::LightWavePuls()
		: _localTime(0.f)
		, _aplphaTimeScale(2.f)
		, _puls(false)
	{
	
	}

	bool LightWavePuls::Update(float dt)
	{
		_localTime += dt*time_scale;
		if(_puls)
		{
			timer = 0.05f*sinf(_localTime*6.f) + 0.005f*sinf(_localTime*11.111f + 1.345f);
			timer = timer + 0.9f;
		}else{
			timer = math::clamp(0.f, 1.f, _localTime);
		}
		if(!finished && _alpha < 1)
		{
			_alpha += dt*_aplphaTimeScale;	
			if(_alpha >= 1)
			{
				_alpha = 1.f;
			}
		}else if(finished && _alpha > 0)
		{
			_alpha -= dt*_aplphaTimeScale;
			if(_alpha <= 0)
			{
				_alpha = 0.f;
			}
		}
		timer *= _alpha;
		return _alpha <= 0.f;
	}

	/*****************************/
	// LightWaves
	/*****************************/
	void LightWaves::Init()
	{
		_shader = Core::resourceManager.Get<Render::ShaderProgram>("CardLightWave");
		MyAssert(_shader);
	}

		
	void LightWaves::Release()
	{
		for(std::list<LightWave*>::iterator i = _waves.begin(); i != _waves.end();i++)
		{
			delete *i;
		}
		_waves.clear();
	}

	void LightWaves::Add(LightWave* wave)
	{
		_waves.push_back(wave);
	}
	void LightWaves::Draw()
	{
		_shader->Bind();
		for(std::list<LightWave*>::iterator i = _waves.begin(); i != _waves.end();)
		{
			(*i)->Draw(_shader);
			i++;
		}
		_shader->Unbind();
	}
	void LightWaves::Update(float dt)
	{
		for(std::list<LightWave*>::iterator i = _waves.begin(); i != _waves.end();)
		{
			if((*i)->Update(dt))
			{
				delete *i;
				i = _waves.erase(i);
			}else{
				i++;
			}
		}
	}

	LightWave* LightWaves::AddShortWave(const FPoint pos_, const float w_, const float r_,  const bool add_, const bool fill_center, const float ts, const float ga, const float delay)	
		
	{
		LightWave* wave = new Card::LightWave();

		wave->pos = pos_;
		wave->width = w_;
		wave->SetRadius(r_);
		wave->add = add_;
		wave->fill = fill_center;
		wave->time_scale = ts;
		wave->globalAlpha = ga;
		wave->timer = -delay;

		Add(wave);
		return wave;
	}
}//namespace Card