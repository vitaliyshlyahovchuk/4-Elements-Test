#pragma once
#include "Game.h"
#include "GameFieldController.h"

namespace Game
{

	class SpiritStrip
	{
	public:
		VertexBufferIndexed _buffer;

		float _stripTime;
		int _numSectors;

		//относительная длинна шлейфа (относительно 1 - вся длина пути)
		float _lenght_uv;
		float _countStart;

		SplinePath<math::Vector3> _path;
		float _fullLenght;
		float _currentStripLenght, _usedStripLenght;
	public:

		SpiritStrip();
		void Clear();
		void SetSplinePath(SplinePath<math::Vector3> &path, float strip_relative_lenght);

		void Draw(FRect frect, float alpha, bool need_morphing = false, FRect rect = FRect(), FPoint morphing_pos = FPoint(), float morphing_angle = 0.f);
		void setStripTime(float stripTime, float scale_lenght, float offset_back);
		float getHeadTime();

		FPoint getStripPartPosition(float t);
		FPoint getStripPosition();
		FPoint getStripGradient();
		FPoint getStripGradient(float t, float limit);
	};


	class Spirit
		: public GameFieldController
	{
		enum State
		{
			SPIRIT_DELAY,
			SPIRIT_APPEAR,
			SPIRIT_FLY,
			SPIRIT_DISAPPEAR,
			SPIRIT_FINISHED,
		}_state;

		bool _finishEffectIsRun;
		bool _hangIsSend;

		float _timerState, _timerScale;
		float _localTime;
		FPoint _pos;
		Render::Texture *_texBody, *_texWing, *_texFlap, *_texEyeLeft, *_texEyeRight, *_texStrip, *_texBrow;

		std::string  _bonusChip;
		Game::Hang _bonus;
		FPoint _posFrom, _posFly, _finishPos;

		AddressVector _chipSeq;

		Game::Square *_destSquare;
		SplinePath<FPoint> _spline;

		//Начальная фаза-смещение вращения вокруг
		float _startWaitOffset;

		EffectsContainer _effCont;
		SpiritStrip _strip;
		SplinePath<float> _angles;
		SplinePath<float> _startAcceleration; //Стартовый разгон
		SplinePath<float> _finishAcceleration; //Финишное торможение

		ParticleEffectPtr _endEffect, _stripEffect;

		FPoint _lastAppearDir; //Направление для иннертного торможениия и возврата

		float _eyesDistance;
		float _eyesAngle;

		float _eyeTimer;
		FPoint _bodyByEyeOffset;
		float _eyeTimeScale;
		bool _eyeOutro;
		
		struct EyeBehaviour
		{
			float time;
			FPoint dir;
			bool change_eye_textures;

			EyeBehaviour(float t, FPoint p, bool change_eye_textures_ = false);
		};
		FPoint _eyePos, _eyePosLast;
		std::list<EyeBehaviour> _eyeBehaviour;

		Render::Texture *_texEyeLeft2, *_texEyeRight2;
		float _timerEyeVariant;
		bool _eyeVariant2Enable;
		bool _showChip;

		FRect _chipFRect;

		long _idSample; //ид звука который сейчас мграется

	private:
		FPoint GetNearOffset();
		FPoint DrawEyes();
		void DrawWingsRight(float t_inert, float t_acceletate);
		void UpdateEyes(float dt);
		void InitEye(float max_wait_time);
		bool CheckSquareDest();
		FRect _uv_rect;

		Game::ChipColor _chip;
		bool _chipInIce;

		bool _autoTrigger;
	public:
		Spirit(FPoint pos, IPoint index_from, const std::string& bonusChip, const Game::Hang& bonus, const AddressVector& chipSeq, Game::FieldAddress to = Game::FieldAddress(-1,-1), float pause = 0.f, int color = -1, Game::ChipColor chip = Game::ChipColor(-1), bool in_ice_chip = false);
		~Spirit();
		void Update(float dt);
		void DrawAbsolute();
		bool isFinish();
		bool InitHangState();
		void SetAutoTrigger();
	public:

		static void ResetCounters();

		// летит ли какой-нибудь бонус к данной клетке
		static bool BonusIsFlyingToCell(Game::FieldAddress fa);

		// летит ли какой-нибудь бонус куда-нибудь вообще
		static bool BonusIsFlying();
	};
}//namespace Game