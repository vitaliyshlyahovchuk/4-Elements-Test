#include "stdafx.h"
#include "CardFlashAnimationPlayer.h"
#include "MyApplication.h"

namespace Card {

	CardFlashAnimationPlayer::CardFlashAnimationPlayer(const std::string &name_lib, const std::string &name_anim, FPoint pos, bool mirror)
		: _animation(new FlashAnimation(name_lib, name_anim))
		, _pos(pos)
		, _mirror(mirror)
	{
		_animation->GetMovieClip()->setPlaybackOperation(GameLua::getPlayOnceOperation());
		Reset();
	}

	void CardFlashAnimationPlayer::Update(float dt)
	{
		_animation->Update(dt);
	}

	void CardFlashAnimationPlayer::Draw()
	{
		_animation->Draw(_pos.x, _pos.y, _mirror);
#ifdef _DEBUG
		Render::FreeType::BindFont("debug");
		Render::PrintString(_pos.x, _pos.y, Int::ToString(_animation->GetCurrentFrame()), 1.f, LeftAlign, BottomAlign);
#endif
	}

	void CardFlashAnimationPlayer::Reset()
	{
		_animation->Reset();
	}

	bool CardFlashAnimationPlayer::isFinish()
	{
		return _animation->IsLastFrame();
	}

}