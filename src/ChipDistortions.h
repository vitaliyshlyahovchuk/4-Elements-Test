#pragma once
#include "types.h"

namespace Game
{
	struct ChipDistortion
		: RefCounter
	{
		float _timer, _timerScale;
		virtual bool Update(float dt) = 0;
		//Сначала было "слово": сделаем только искажение квадрата отрисовки
		virtual void CorrectRect(FRect &rect) const = 0;
		//Затем это все понадобилось украсить, выводом из альфы и показом глаз
		virtual float GetAlpha() const;
		virtual bool ShowEye() const;
	};

	struct ChipJumpOnHang
		: public ChipDistortion
	{
		ChipJumpOnHang(float pause, float time);
		bool Update(float dt);
		void CorrectRect(FRect &rect) const;
	};

	struct ChipDistortionFlyAppear
		: public ChipDistortion
	{
		ChipDistortionFlyAppear(float pause, float time);
		bool Update(float dt);
		void CorrectRect(FRect &rect) const;
		float GetAlpha() const;
		bool ShowEye() const;
	};
	
	struct ChipDistortionFlyAppearStop
		: public ChipDistortion
	{
		ChipDistortionFlyAppearStop(float pause, float time);
		bool Update(float dt);
		void CorrectRect(FRect &rect) const;
		float GetAlpha() const;
		bool ShowEye() const;
	};

	struct ChipDistortionFlyHide
		: public ChipDistortion
	{
		ChipDistortionFlyHide(float pause, float time);
		bool Update(float dt);
		void CorrectRect(FRect &rect) const;
		float GetAlpha() const;
		bool ShowEye() const;
	};

	struct ChipAppearFromGround
		: public ChipDistortion
	{
		ChipAppearFromGround(float pause, float time);
		bool Update(float dt);
		void CorrectRect(FRect &rect) const;
		float GetAlpha() const;
	};

}//namespace Game