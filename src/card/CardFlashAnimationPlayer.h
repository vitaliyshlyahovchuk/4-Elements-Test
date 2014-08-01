#pragma once

#include "Flash/bindings/FlashAnimation.h"

namespace Card 
{
	class CardFlashAnimationPlayer {
	private:
		FlashAnimation *_animation;
		FPoint _pos;
		bool _mirror;
	public:
		CardFlashAnimationPlayer(const std::string &name_lib, const std::string &name_anim,  FPoint pos, bool mirror = false);
		void Update(float dt);
		void Draw();
		void Reset();
		bool isFinish();
	};
}