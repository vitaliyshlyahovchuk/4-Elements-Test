#pragma once
#include "GameFieldController.h"

namespace Game
{
	void InitArrowEffects(rapidxml::xml_node<> *xml_info);

	extern float ARROW_BONUS_SPEED;

	class ArrowBonusController : public GameFieldController
	{
		struct Arrow
		{
			math::Vector3 start_pos;
			math::Vector3 dir;
			Byte _dir_type;
			float radius;
			float _lenght;
			int chips;
			float next_lenght;
			float _stripTime;
			float local_time, time_scale;
			ParticleEffectPtr _effFront, _effBack;
			bool _effFrontIsRan;
			float _alphaFrontEffect;

			Arrow(IPoint start, IPoint dir, int r, float time_scale_, Byte dir_id);
			~Arrow();
			void OnFinish();
			bool Update(float dt);
			void Draw();
		};
		FPoint _start_pos;;
		std::string _effectName;
		std::vector<Arrow> _arrows;
		bool is_finished;
		float _effect2Timer;
		bool _effect2Runing;
	public:
		ArrowBonusController(float startTime, FPoint start_pos, int type);
		void AddArrow(IPoint start, IPoint dir, int radius, Byte dir_id);
		void Update(float dt);
		void Draw();
		bool isFinish();
	};
}