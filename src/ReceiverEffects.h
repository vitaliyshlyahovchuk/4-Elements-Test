#pragma once
#include "GameFieldController.h"

namespace Receiver
{
	class ReceiverEffect
		: public GameFieldController
	{
		bool _finishEffectRunning;
		enum State
		{
			RE_JUMP,
			RE_STAY,
			RE_FLY,
			RE_HIDE,
			//RE_WAIT,
			RE_FINISH,
		}_state;
		FPoint _lootPos;
		float _jumpTmeScale, _stayTimeScale, _flyTimeScale;
		float _jumpScale;
		EffectsContainer _effCont, _effContInner;
		ParticleEffectPtr _flyEffect;
		FPoint _v0;
		float _time, _localTime;
		SplinePath<FPoint> _spline;
		SplinePath<float> _angleSpline;
		std::string _uid;
		FPoint _posStartOnField;
		FPoint _finishScaleCrystal;
		float _lastAngle;

		//Render::Texture *_texture;
		Render::StreamingAnimationPtr _crystalAnim;
		//long _idSample; //ид трека

		int _frame1, _frame2;
		float _crystalTime, _crystalTimeScale;
	public:
		static bool keep_camera;
	public:
		ReceiverEffect(const FPoint pos, std::string _uid, int current_frame);
		~ReceiverEffect();

		void Update(float dt);
		void DrawAbsolute();
		bool isFinish();

	};
}//namespace Receiver