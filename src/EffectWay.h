#pragma once
#include "GameFieldController.h"
#include "Game.h"

namespace Utils
{
	class EffectDrawer
		: public GameFieldController
	{
		float _drawTime, _drawTimeScale;
		float _alpha;
		EffectsContainer _effCont;
		DrawTypes _drawType;
		Color _color;
	public:
		EffectDrawer(std::string name, ParticleEffectPtr &eff, const Color color, bool need_square_scale = true);
		void SetDrawType(DrawTypes type, const float time, const float pause);
		void Update(float dt);
		virtual bool isFinish();
		void DrawBase();
		virtual void Draw();
		virtual void DrawUnderChips();
	};

} //namespace Utils