#pragma once

#include "Flash.h"

class UserAvatar;

class FlashUserAvatar: public FlashDisplayObject
{
public:
	FlashUserAvatar(UserAvatar* avatar);
	virtual void render(FlashRender& render);
	virtual bool hitTest(float x, float y, IHitTestDelegate* hitTestDelegate);
	virtual bool getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem);
private:
	UserAvatar* avatar;
	Render::Texture* mask;
};