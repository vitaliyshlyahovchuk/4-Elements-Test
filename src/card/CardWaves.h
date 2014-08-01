#pragma once
namespace Card
{
	struct LightWave
	{
		//Волна с постоянной шириной волны.
	protected:
		float D;
		float _alpha;
	public:
		float globalAlpha;
		float timer, time_scale;
		FPoint pos;
		float width;
		bool fill;
		bool add;
		bool finished;
		bool scaling;

		LightWave();
		virtual ~LightWave();
		virtual void Draw(Render::ShaderProgram *shader);
		virtual bool Update(float dt);
		void SetRadius(const float r);
	};

	struct LightWavePuls
		: public LightWave
	{
		float _aplphaTimeScale;
		float _localTime;
		bool _puls;

		LightWavePuls();
		virtual bool Update(float dt);
	};

	class LightWaves
	{
		std::list<LightWave*> _waves;
		Render::ShaderProgram *_shader;
	public:
		void Init();
		void Release();
		void Add(LightWave* wave);
		LightWave* AddShortWave(const FPoint pos_, const float w_, const float r_,  const bool add_ = false, const bool fill_center = false, const float ts = 1.f, const float ga = 1.f, const float delay = 0.f);
		void Draw();
		void Update(float dt);
	};
	extern LightWaves lightWaves;
}//namespace Card
