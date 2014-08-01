#pragma once
#include "GameFieldController.h"

namespace Match3
{
	class FlyStarArrowEffect
		: public GameFieldController
	{
		enum State
		{
			FLYSTAR_PAUSE,
			FLYSTAR_JUMP_TO_CENTER,
			FLYSTAR_FLY,
			FLYSTAR_HIDE,
			FLYSTAR_WAIT,
			FLYSTAR_FINISH,
		}_state;
		float _pause;
		EffectsContainer _effCont, _effContInner;
		ParticleEffectPtr _flyEffect;
		FPoint _v0, _centerPoint;
		static float ttt;
		float _time, _localTime;
		SplinePath<FPoint> _spline;
		Render::Texture *_texture;
		FPoint _centerTexture;
		FPoint _posStartOnField;
		Message _messageEnd;
		GUI::Widget *_messageWidget;
		float _endAngle;
		FPoint _offset_in_cell;
	public:
		FlyStarArrowEffect(const FPoint pos, const FPoint offset_in_cell, Render::Texture *texture, const float angle, float pause);
		~FlyStarArrowEffect();

		void Update(float dt);
		void DrawAbsolute();
		bool isFinish();
		void AddMessage(const Message &message, GUI::Widget *widget);

	};
}//namespace Match3