#include "stdafx.h"
#include "FlashUserAvatar.h"
#include "FriendInfo.h"

FlashUserAvatar::FlashUserAvatar(UserAvatar* avatar)
	: FlashDisplayObject()
	, avatar(avatar)
	, mask(Core::resourceManager.Find<Render::Texture>("UserAvatarMask"))
{
}


void FlashUserAvatar::render(FlashRender& render) {
	render.flush();
	FPoint position(0.f,0.f);
	localToGlobal(position.x, position.y);
	float alpha = getAlpha();
	float scaleX(0.f), sx(0.f), scaleY(0.f), sy(0.f);
	getScale(scaleX, scaleY);
	IFlashDisplayObject* p = getParent();
	while (p) {
		alpha *= p->getAlpha();
		p->getScale(sx, sy);
		scaleX *= sx;
		scaleY *= sy;
		p = p->getParent();
	};
	
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(position.x, position.y, 0.f));
	Render::device.MatrixScale(scaleX, -1.f * scaleY, 1.f);
	Render::BeginAlphaMul(alpha);
	if (!mask) {
		avatar->Draw(FPoint(0.f, 0.f));
	} else {
		avatar->GetTexture()->Bind(0);
		mask->Bind(1, Render::STAGE_C_SKIP + Render::STAGE_A_REPLACE);
		Render::DrawQuad(-25.f, -25.f, 50.f, 50.f, FRect(0.f, 1.f, 0.f, 1.f), FRect(0.f, 1.f, 0.f, 1.f));
	}
	Render::EndAlphaMul();
	Render::device.PopMatrix();
	render.invalidateConstant(FlashTextureCh0);
}

bool FlashUserAvatar::hitTest(float x, float y, IHitTestDelegate* hitTestDelegate) {
	return false;
}

bool FlashUserAvatar::getBounds(float& left, float& top, float& right, float& bottom, IFlashDisplayObject* targetCoordinateSystem) {
	float x,y;
	getPosition(x, y);
	if ( targetCoordinateSystem == NULL ){
		left = x;
		top = y;
		right = x + avatar->GetWidth();
		bottom = y + avatar->GetHeight();
	}else{
		float tx, ty;
		tx = x;
		ty = y;
		localToTarget(tx, ty, targetCoordinateSystem);
		left = right = tx;
		top = bottom = ty;

		tx = x + avatar->GetWidth();
		ty = y;
		localToTarget(tx, ty, targetCoordinateSystem);
		if (tx < left) {       left = tx; }
		else if (tx > right) { right = tx; }
		if (ty < top) { top = ty; }
		else if (ty > bottom) { bottom = ty; }
		
		tx = x;
		ty = y + avatar->GetHeight();
		localToTarget(tx, ty, targetCoordinateSystem);
		if (tx < left) {       left = tx; }
		else if (tx > right) { right = tx; }
		if (ty < top) { top = ty; }
		else if (ty > bottom) { bottom = ty; }

		tx = x + avatar->GetWidth();
		ty = y + avatar->GetHeight();
		localToTarget(tx, ty, targetCoordinateSystem);
		if (tx < left) {       left = tx; }
		else if (tx > right) { right = tx; }
		if (ty < top) { top = ty; }
		else if (ty > bottom) { bottom = ty; }
	}
	return true;
}